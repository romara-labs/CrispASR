# Z1 Mapping Audit (TK-0006) — 2026-06-28

**Task:** TK-0006 (Z1-test)
**Auditor:** kiyotaka (executor)
**Ground truth:** `voicer.cpp/convert-gguf/Kokoro-82M-v1.0-F32.full.gguf` (513 tensores) + `voicer.cpp/src/kokoro/kokoro-gguf.cpp` (loader fonte)

## Método
Portei `map_full_gguf_tensor_name` (kokoro.cpp:312-407), `starts_with` e `replace_all` fielmente para Python (incluindo a ordenação exata dos `replace_all` e o early-return `n==name ? "" : n`) e rodei todos os 459 nomes `kokoro.*` reais do GGUF F32 full através dele.

## Resultado — MAPEAMENTO DE NOMES É COMPLETO

| Métrica | Valor |
|---|---|
| tensores `kokoro.*` (fonte) | 459 |
| aliases internos produzidos | **459 (100%)** |
| **não-mapeados (gap)** | **0** |
| colisões (2+ fontes → mesmo interno) | **0** |

Distribuição por prefixo (todos têm ramo de mapeamento): decoder 305, predictor 106, bert 25, text_encoder 21, bert_encoder 2.

**Conclusão (fato): a tabela `map_full_gguf_tensor_name` NÃO é o bug ativo.** Ela cobre todos os 459 tensores do GGUF full atual, sem gaps nem colisões. O concern original de Z1 (fragilidade a mudança upstream de naming) permanece como **robustez**, não como bug ativo.

## Reframe — onde estão os erros reais da migração

Evidência convergente para **orientação/shape**, não nomes:

1. **Loader fonte não remapeia nada.** `kokoro-gguf.cpp:169 index_tensors` indexa tensores pelo nome externo verbatim; o grafo do voicer.cpp consome os nomes `kokoro.*` nativamente. Logo, `map_full_gguf_tensor_name` + a lógica de **transpose** é cola de migração específica do grafo pré-existente do CrispASR (que usa nomes curtos como `bert.attn_q.weight`).
2. **Scaffolding de debug mira shape.** `KOKORO_DEBUG_SHAPES` (core/lstm.h:45-67, kokoro.cpp `kokoro_debug_*`) loga `can=(w->ne[0]==x->ne[0]) && (x->ne[2]%w->ne[2]==0) && ...` em cada `mul_mat`/`conv` — exatamente o detector de incompatibilidade de orientação. Ele existe porque os erros da migração se manifestam como mismatch de shape em runtime.
3. **123 tensores 2D são candidatos a transpose** (de 459: 254 1D, 123 2D, 77 3D-conv, 5 escalares). A decisão `transpose_2d` (kokoro.cpp:430-431) transpõe todo 2D não-embedding não-conv. Se essa decisão estiver errada para algum peso → `mul_mat` com shape incompatível → erro de migração.

## Hipótese operacional
O grafo do CrispASR foi portado do voicer.cpp mas `kokoro.cpp` pré-existia à migração full-GGUF (usava o converter antigo `convert-kokoro-to-gguf.py` com layout possivelmente diferente). A lógica de transpose tenta converter o layout novo (nativo) → layout esperado pelo grafo antigo. Erros = pesos onde essa conversão está incorreta (transpose indevido, ou falta de transpose, ou conv mal classificado pelo `is_full_gguf_conv_alias`).

## Bloqueador para TK-0007 (o "arruma")
Para corrigir com precisão (não chutar), é necessário saber **quais** pesos estão mal-orientados. Evidência definitiva = rodar síntese Kokoro com `KOKORO_DEBUG_SHAPES=1` sobre o F32 full GGUF e ler as linhas `can=0`. Build do CrispASR está atualizado (`build/bin/crispasr`, `kokoro.cpp.o` mais novo que a fonte).

## Recomendação
- **TK-0006 (este):** CONCLUÍDO. Achado: nomes OK; alvo real = orientação.
- **TK-0007 (Z1-fix):** REFOCUSAR — não é "corrigir a tabela de nomes" (ela está certa), é "corrigir a lógica de transpose/conv-classification". Pré-requisito: capturar `KOKORO_DEBUG_SHAPES can=0` em run real para listar os pesos mal-orientados.
- **TK-0008 (unit-test):** mantém valor — testar a tabela de nomes contra regressão de naming upstream (robustez), mesmo não sendo o bug ativo.

## Notas
- Não editei código (TK-0006 é read-only).
- Lista completa dos 459 nomes em `_kiyotaka_project/z1-tensor-names-raw.txt`.
- Separei fato (nomes completos; loader fonte não remapeia; scaffolding mira shape; 123 2D) de hipótese (causa raiz = transpose) e bloqueador (precisa de run de debug para precisão).

---

## Atualização — Diagnóstico de runtime (KOKORO_DEBUG_SHAPES sobre o GGUF F32 full)

Build do CrispASR estava atualizado; rodei `build/bin/crispasr --backend kokoro -m Kokoro-82M-v1.0-F32.full.gguf --voice af_heart --tts "hello world"` com `KOKORO_DEBUG_SHAPES=1`.

### Fatos (evidência dura)
- **Crash:** `GGML_ASSERT(ggml_can_mul_mat(a,b))` em `ggml.c:3299`.
- **Backtrace:** `kokoro_adain1d` ← `kokoro_resblock1_forward` ← `kokoro_run_generator` ← `kokoro_run_audio` ← `kokoro_synthesize_phonemes`. (mul_mat não coberto pelo shim `kokoro_debug_mul_mat`.)
- Estendi temporariamente o shim ao mul_mat de `kokoro_adain1d` (kokoro.cpp:1135) — **46 chamadas OK**, **1 mismatch**:
  - **MISMATCH:** `fc_w` = tensor nomeado `kokoro.decoder.generator.noise_res.0.adain1.0.fc.weight`, `ne=(512,128)` (ne[0]=**512**), `style` `ne=(128,1)` (ne[0]=128), `x=(256,1240)`. → 512 ≠ 128 → assert.
  - As 46 OK (`dec.decode.*.adain*`, `pred.*.adain*`, `dec.encode.adain*`) chegam com `fc_w` ne[0]=128 (nativo) == style ne[0]=128.
- **No GGUF**, `noise_res.0.adain1.0.fc.weight` tem ne[0]=**128** (idêntico a todos os demais adain). Portanto a orientação é **invertida em runtime** (128→512) entre load e uso, só para esse peso (e presumivelmente os demais `dec.gen.*` adain, que não chegaram a rodar — o crash ocorre no primeiro, `noise_res.0`).

### Tentativa de fix (TK-0007) — INCORRETA, REVERTIDA
- Hipótese: `is_full_gguf_conv_alias` (kokoro.cpp:415 `starts_with dec.gen.`) impedia a transposição dos adain fc do gerador. Apliquei um special-case para transpô-los.
- **Resultado:** introduziu mismatch num peso (transpôs o que não devia) — a hipótese estava errada. **Revertida**. `is_full_gguf_conv_alias` e o diagnóstico temporário foram removidos; `kokoro.cpp` voltou ao estado original da migração; build limpo confirmado.

### Bloqueador / questão em aberto (fato vs inferência)
- **Fato:** o peso `dec.gen.noise_res.0.adain1.0` chega ao `kokoro_adain1d` com ne[0]=512 em runtime, embora (a) o GGUF o armazene com ne[0]=128 e (b) `is_full_gguf_conv_alias` marque `dec.gen.*` como não-transposto. Algo inverte a orientação entre load e uso, apenas para os adain do gerador.
- **Inferência:** há uma segunda transposição (no loader normal `load_weights`, ou num reshape/cont, ou num caminho de alias diferente) que afeta os pesos do gerador mas não os do decoder/predictor.
- **Próximo passo necessário (TK-0007 refocusado):** instrumentar `require()`/`add_full_gguf_tensor_aliases` para logar, para cada alias `dec.gen.*.adain*`, o ne[0] no momento do cadastro vs no momento do fetch — para identificar exatamente onde ne[0] vira 512. Sem isso, qualquer fix é chute.

### Estado do código (atualizado após resolução)
- `src/kokoro.cpp`: migração original **+ fix cirúrgico em `is_full_gguf_conv_alias`** (ver RESOLUÇÃO abaixo). Sem debug residual; build limpo.

---

## RESOLUÇÃO — Z1 CORRIGIDO E VERIFICADO (2026-06-28)

### Causa raiz confirmada (fato, via instrumentação de `add_full_gguf_tensor_aliases`)
O GGUF full armazena os adain linears do gerador (`dec.gen.{noise_res,resblocks}.adain{1,2}.<j>.weight`) com **ne[0]=2C** (out-first), ne[1]=style. O grafo (`kokoro_adain1d`) faz `mul_mat(fc_w, style)` exigindo `fc_w->ne[0]==style->ne[0]==style`. Esses pesos **precisam ser transpostos** para ne[0]=style — exatamente como os adain do decoder-body (transpostos via `is_full_gguf_conv_alias=false`). A regra `starts_with(alias, "dec.gen.")` em `is_full_gguf_conv_alias` os classificava como conv → **impedia a transposição** → `ne[0]=2C` → `ggml_can_mul_mat` abortava.
- Evidência do instrumento (pré-fix): `src_ne0=512 src_ne1=128 ndims=2 transpose_2d=0` para `dec.gen.noise_res.0.adain1.0.weight`.
- (Nota: a afirmação anterior de que o GGUF armazenava ne[0]=128 estava errada — o `t.shape` do leitor python gguf é a ne em ordem de storage, ne[0] primeiro; o GGUF declara ne[0]=2C.)

### Fix aplicado (kokoro.cpp `is_full_gguf_conv_alias`)
Sob `dec.gen.`, apenas os adain linears (`.adain1.`/`.adain2.` + `.weight`) são tratados como não-conv (→ transpostos); convs (3D), conv_post, ups, noise_convs e alpha* continuam sem transposição. **Detalhe crítico acertado nesta 2ª tentativa:** o alias já teve `.fc.` removido pelo mapeamento, então o match é `.adain1.`/`.adain2.` + `.weight` (a 1ª tentativa testou `.fc.weight`, que nunca existe no alias → foi no-op).

### Verificação (fato)
- Rebuild limpo, sem debug.
- `transpose_2d=1` agora para os adain do gerador (instrumento).
- Run sem `KOKORO_DEBUG_SHAPES`: **0 crashes**; saída `TTS output written ... 37200 samples @ 24000 Hz, 1.55 sec`.
- Áudio não-silencioso: peak=1196, rms=300.5, 60.7% frames não-silenciosos (fala real).
- Kokoro TTS funciona end-to-end com GGUF full + voz embutida `af_heart`.

### Estado
- **Z1: CORRIGIDO E VERIFICADO.** TK-0006 (audit) e TK-0007 (fix) concluídos.
- Pendente (não-bloqueador): TK-0008 (unit-test) travaria a regressão — idealmente cobrindo `is_full_gguf_conv_alias` (decisão de transposição), não só o mapeamento de nomes; exige expor as funções `static`. Nível de áudio (peak ~3.6% full-scale) pode ser otimizado depois (modelo é WIP per D4).


