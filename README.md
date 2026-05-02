# VU Meter com Arduino Nano + LCD 16x2 I2C + Microfone Eletreto

Projeto de um VU meter horizontal usando:
- **Arduino Nano**
- **LCD 16x2 azul com módulo I2C (PCF8574)**
- **Módulo de microfone eletreto com pré-amplificador e trimpot**

O display usa as **16 colunas x 2 linhas** como duas barras horizontais de VU.

## Arquivos

- `vu_meter_nano.ino`: código principal em C++/Arduino (somente I2C).

## Como funciona

1. Ao ligar, roda um **teste visual do LCD** com blocos aleatórios e flashes.
2. No loop principal, lê o sinal analógico do microfone e calcula a amplitude pico-a-pico.
3. Mapeia essa amplitude para níveis de 0 a 16 e desenha as barras nas 2 linhas.

---

## Ligações (somente I2C)

### LCD 16x2 I2C

- VCC -> 5V
- GND -> GND
- SDA -> A4 (Arduino Nano)
- SCL -> A5 (Arduino Nano)

### Microfone eletreto (módulo com pré)

- VCC -> 5V
- GND -> GND
- AO (analógico) -> A0

> Ajuste o trimpot do módulo de microfone para encontrar a sensibilidade ideal.

---

## Endereço I2C do LCD

No código, o endereço padrão está como **0x27**:

```cpp
LiquidCrystal_I2C lcd(0x27, 16, 2);
```

Se o seu módulo não responder, troque para **0x3F**.

---

## Biblioteca necessária

Instale a biblioteca **LiquidCrystal_I2C** na Arduino IDE.

---

## Alimentação

- Pode alimentar via USB do Nano durante testes.
- Em fonte externa, garanta 5V estáveis e GND comum entre todos os módulos.

---

## Compilação e upload

Na Arduino IDE:
1. Selecione placa: **Arduino Nano**.
2. Selecione processador correto (ATmega328P ou Old Bootloader, conforme sua placa).
3. Instale a biblioteca `LiquidCrystal_I2C`.
4. Abra `vu_meter_nano.ino`.
5. Compile e faça upload.

---

## Ajustes finos sugeridos

- Alterar sensibilidade em `levelFromAmplitude()` (faixa `0..350`).
- Ajustar suavização em `smoothed = 0.75 * ... + 0.25 * ...`.
- Mudar tempo de atualização em `REFRESH_MS`.
- Mudar janela de amostragem em `SAMPLE_WINDOW_MS`.
