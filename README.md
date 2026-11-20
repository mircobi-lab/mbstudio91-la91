# MBStudio91 LA-91

Sintetizzatore a sintesi LA (Linear Arithmetic) ispirato al D-50, con pannello da programmatore a cursori in stile hardware MBStudio91. Formati: VST3, AU (Mac) e Standalone.

La mappa dei parametri segue DSynkant di Nil Geisweiller (https://github.com/ngeiswei/dsynkant, GPL v2 o successiva): stessi valori e stesso ordine dei byte del dump SysEx. Il motore audio è nuovo: DSynkant contiene il modello dei parametri ma non genera suono.

## Cosa c'è

- 4 partial per patch (UPPER 1-2, LOWER 1-2) con le 7 STRUCTURE, ring modulator compreso.
- Partial SYNTH: dente di sega o quadra anti-aliasing, PW con velocity, aftertouch e LFO, filtro TVF risonante con key follow, bias, inviluppo a 5 tempi, LFO.
- Partial PCM: 100 forme d'onda generate dal plugin (attacchi, loop, sequenze). Non ci sono campioni Roland.
- TVA con inviluppo a 5 tempi, velocity, bias, tremolo LFO. Pitch envelope, 3 LFO, EQ a due bande e chorus a 8 tipi per ogni tone.
- Key mode WHOLE / DUAL / SPLIT (più le varianti SOLO), portamento, hold, pitch bend, mod wheel, aftertouch.
- 32 tipi di riverbero/delay (approssimazioni) e volume totale.
- 17 preset di fabbrica MBStudio91 originali.
- Import di banchi `.syx` del D-50 (dump DT1, fino a 64 patch). Nomi e valori vengono letti esattamente; le parti PCM usano le forme d'onda sostitutive, quindi il suono è simile ma non identico.

## Pannello

- COMMON SELECT (UPPER/LOWER): sceglie il tone controllato dai cursori marcati con LED verde (STRUCTURE, PITCH MOD, PITCH ENV, LFO, EQ, CHORUS).
- PARTIAL SELECT: sceglie il partial controllato dai cursori con LED blu (WG, TVF, TVA e i loro inviluppi).
- PARTIAL ON: accende o spegne i 4 partial.
- Il display mostra il parametro che stai muovendo e il suo valore. Doppio clic su un cursore lo riporta al valore iniziale.
- LOAD SYX carica un banco, CLEAR lo svuota. Il banco importato resta salvato nel progetto della DAW.

## Limiti attuali (prototipo 0.1)

- CHASE e i parametri MIDI di trasmissione non sono gestiti.
- SEPARATE si comporta come SPLIT.
- I tempi degli inviluppi, i livelli e i tipi di riverbero sono tarati a orecchio sul manuale e non sono misurati su uno strumento reale.

## Compilare su Mac

Servono Xcode e CMake (`brew install cmake`).

```
cmake -S . -B build -G Xcode
cmake --build build --config Release --target MBStudio91LA91_VST3 MBStudio91LA91_AU MBStudio91LA91_Standalone
```

JUCE 9.0.5 si scarica da solo. Per usare una copia locale aggiungi `-DJUCE_DIR=/percorso/JUCE`.

I file compilati sono in `build/MBStudio91LA91_artefacts/Release/`. Copiali così:
- `MBStudio91 LA-91.vst3` in `~/Library/Audio/Plug-Ins/VST3/`
- `MBStudio91 LA-91.component` in `~/Library/Audio/Plug-Ins/Components/`
- `MBStudio91 LA-91.app` in `Applicazioni`

Se carichi il progetto su GitHub, il workflow in `.github/workflows/build.yml` compila da solo le versioni per Mac (universale Intel/Apple Silicon) e Windows.

## Licenza

LA-91 è distribuito sotto GNU AGPL v3 (vedi `LICENSE.md`), compatibile con JUCE (AGPL v3) e con DSynkant (GPL v2 o successiva, vedi `COPYING-DSynkant`). Font Bebas Neue e Antonio sotto SIL Open Font License (`MBStudioUI/fonts/OFL-*.txt`).

D-50, PG-1000 e Roland sono marchi dei rispettivi proprietari. Sono citati solo per descrivere la compatibilità dei dati: LA-91 non è affiliato a Roland.
