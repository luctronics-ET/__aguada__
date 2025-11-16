# ESP32-C3 Super Mini - Pinout Reference

**Referência oficial:** https://www.espboards.dev/esp32/esp32-c3-super-mini/

## 📋 Especificações

- **MCU**: ESP32-C3FH4 (QFN32, RISC-V 32-bit, single core @ 160MHz)
- **Flash**: 4MB embutida (XMC)
- **RAM**: 400KB SRAM
- **WiFi**: 2.4GHz 802.11 b/g/n
- **Bluetooth**: BLE 5.0
- **USB**: USB-C com USB-Serial/JTAG integrado
- **Tamanho**: 22.52 x 18mm

## 🔌 Pinout Completo

### Pinos Digitais Disponíveis
| GPIO | Funções Alternativas         | Notas                           |
|------|------------------------------|---------------------------------|
| 0    | ADC1_CH0, XTAL_32K_P         | 📊**Echo Ultra-01**             |
| 1    | ADC1_CH1, XTAL_32K_N         | 📊**Trigger Ultra-01**          |
| 2    | ADC1_CH2, FSPIQ              |     📊**Echo Ultra-02**         |
| 3    | ADC1_CH3, FSPIHD             |     📊**Trigger Ultra-02**      |
| 4    | ADC1_CH4, FSPICS0, SCK       |     📊**SOM**                   |
| 5    | ADC2_CH0, FSPIWP   MISO      |     📊**VIN AD**                |
| 6    | FSPICLK, MTCK      MOSI      |                                 |
| 7    | FSPID, MTDO        SS        |                                 |
| 8    | **LED_BUILTIN**,             | ⚠️**LED_BUILTIN**,              |
| 9    | I2C_SCL                      | ⚠️**RESERVADO PARA I2C**        |
| 10   | I2C_SDA                      | ⚠️**RESERVADO PARA I2C**        |
| 20   | U0RXD (USB)                  | ❌**Usado por USB-Serial**      |
| 21   | U0TXD (USB)                  | ❌**Usado por USB-Serial**      |

### Pinos de Alimentação
| Pino | Função         | Tensão     |
|------|----------------|------------|
| 5V   | USB Power In   | 5V         |
| 3V3  | Regulador Out  | 3.3V       |  LDO 3.3V, 500mA máximo
| GND  | Ground         | 0V         |

## ⚠️ Notas Importantes

### LED Interno (GPIO8)
- O ESP32-C3 Super Mini possui um **LED azul interno** conectado ao **GPIO8**

### I2C (Futura Expansão)
- **SDA**: GPIO10 (reservado)
- **SCL**: GPIO9 (reservado)
- Planejado para display e sensores adicionais (temperatura, pressão, etc.)

### USB-Serial Integrado
- **NÃO** requer chip externo (CP2102/CH340)
- GPIO20/21 automaticamente usados para USB
- Programação e debug via USB-C direto

### Boot/Reset
- **Botão BOOT**: GPIO9 (pull-up interno)
- **Botão RESET**: EN pin
- Para entrar em modo download: BOOT pressionado + RESET

## 🎯 Uso no Aguada V2

### Pinos Utilizados
```c
#define HC_SR04_ECHO_GPIO  0    // Entrada - Echo do sensor
#define HC_SR04_TRIG_GPIO  1    // Saída - Trigger do sensor
```

### Pinos Disponíveis para Expansão
- **GPIO0-7**:  Livres para uso geral
- **GPIO8**:    Reservado para **LED_BUILTIN**
- **GPIO9-10**: Reservado para **I2C** Display, temp/umid, ultra, 
- **GPIO4-7**:  Reservado para **SPI** Ethernet shield
- **ADC**:      Pinos 0-5 podem ser usados como ADC se necessário

## 📚 Referências

1. **Pinout Oficial**: https://www.espboards.dev/esp32/esp32-c3-super-mini/#onboardLed
2. **Datasheet ESP32-C3**: https://www.espressif.com/sites/default/files/documentation/esp32-c3_datasheet_en.pdf
3. **ESP-IDF Programming Guide**: https://docs.espressif.com/projects/esp-idf/en/latest/esp32c3/

## 🔧 Hardware Notes

### Regulador de Tensão
- **ME6211C33M5G**: LDO 3.3V, 500mA máximo
- Entrada: 5V via USB-C
- Não ultrapassar 500mA no 3V3 pin

### Flash
- **XMC XM25QH32B**: 4MB SPI Flash, 80MHz
- Suporta OTA updates
- 2MB usados por padrão (partition table)

---

**Atualizado:** 13 de outubro de 2025
**Projeto:** Aguada V2 - CMASM 




## Gateway Wifi

## 🔌 Pinout Completo

### Pinos Digitais Disponíveis

| GPIO | Funções Alternativas         | Notas                           |
|------|------------------------------|---------------------------------|
| 0    | ADC1_CH0, XTAL_32K_P         | 📊**VIN AD**             |
| 1    | ADC1_CH1, XTAL_32K_N         |                          |
| 2    | ADC1_CH2, FSPIQ              | 
| 3    | ADC1_CH3, FSPIHD             |           |
| 4    | ADC1_CH4, FSPICS0, SCK       |   **SPI SCK**                    |
| 5    | ADC2_CH0, FSPIWP   MISO      |   **SPI MISO**               |
| 6    | FSPICLK, MTCK      MOSI      |   **SPI MOSI**                             |
| 7    | FSPID, MTDO        SS        |   **SPI SS**                             |
| 8    | **LED_BUILTIN**              | ⚠️**LED_BUILTIN**,              |
| 9    | I2C_SCL                      | ⚠️**I2C SCL**        |
| 10   | I2C_SDA                      | ⚠️**I2C SDA**        |
| 20   | U0RXD (USB)                  | ❌**USB-Serial**      |
| 21   | U0TXD (USB)                  | ❌**USB-Serial**      |

### Pinos de Alimentação
| Pino | Função         | Tensão     |
|------|----------------|------------|
| 5V   | USB Power In   | 5V         |
| 3V3  | Regulador Out  | 3.3V       |
| GND  | Ground         | 0V         |
