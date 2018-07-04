#include <SPI.h>
#include <MFRC522.h>
#include <LiquidCrystal.h>

#define SS_PIN  53
#define RST_PIN 5                               // Pino de Reset do MRF

MFRC522 mfrc522(SS_PIN, RST_PIN);               // Create MFRC522 instance.

#define COMMON_ANODE

#ifdef  COMMON_ANODE
#define LED_ON LOW
#define LED_OFF HIGH
#else
#define LED_ON HIGH
#define LED_OFF LOW
#endif

constexpr uint8_t redLed     = 10;   // Set Led Pins
constexpr uint8_t greenLed   = 11;

const String Versao          = "v1.0";          // Versão do Firmware

const String UidCartaoMestre = "2D C3 76 89";   // UID do Cartao Mestre

const String UidUsuario1     = "EA 30 B2 73";   // UID do Usuario 1
const String UidUsuario2     = "F5 94 BF 65";   // UID do Usuario 2

String CartaoAtual           = "00 00 00 00";
String CartaoAnterior        = "FF FF FF FF";

float SaldoUsuario1 = 0;
float SaldoUsuario2 = 0;

const float valorRecarga     = 10.00;           // Valor da recarga

bool modoRecarga             = false;

MFRC522::MIFARE_Key key;                        // Chave de autenticação MIFARE
MFRC522::StatusCode status;                     // Status

const byte sector         = 1;                  // Setor de leitura/escrita do saldo dos cartões
const byte blockAddr      = 4;                  // Endereço do bloco de leitura/escrita do saldo
const byte trailerBlock   = 7;                  // Endereço do último bloco do setor 1, chamado de trailer

byte dataBlock[16];                             // Vetor de dados a ser lidos/escritos no bloco de memória do cartão PICC

byte buffer[18];
byte size = sizeof(buffer);

void setup()
{
  pinMode(redLed  , OUTPUT);
  pinMode(greenLed, OUTPUT);

  digitalWrite(redLed  , LED_OFF);              // Make sure led is off
  digitalWrite(greenLed, LED_OFF);              // Make sure led is off

  Serial.begin(9600);                           // Inicia a serial
  SPI.begin();                                  // Inicia SPI bus
  mfrc522.PCD_Init();                           // Inicia MFRC522
  Serial.print("Sistema de Catraca ");
  Serial.print(Versao);
  Serial.println("\n");
  Serial.println("Aproxime o seu cartao do leitor...");
  Serial.println();

  // Prepare the key (used both as key A and as key B)
  // using FFFFFFFFFFFFh which is the default at chip delivery from the factory
  for (byte i = 0; i < 6; i++) {
    key.keyByte[i] = 0xFF;
  }

  for (byte i = 0; i < 16; i++) {
    dataBlock[i] = 0x00;
  }
}

void loop()
{
  // Look for new cards
  if ( ! mfrc522.PICC_IsNewCardPresent())
  {
    return;
  }
  // Select one of the cards
  if ( ! mfrc522.PICC_ReadCardSerial())
  {
    return;
  }

  // Authenticate using key A
  status = (MFRC522::StatusCode) mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, trailerBlock, &key, &(mfrc522.uid));
  if (status != MFRC522::STATUS_OK) {
    Serial.print(F("PCD_Authenticate() failed: "));
    Serial.println(mfrc522.GetStatusCodeName(status));
    Serial.println("\nFalha na autenticação. Repita o procedimento\n");
    return;
  }

  // Read data from the block
  status = (MFRC522::StatusCode) mfrc522.MIFARE_Read(blockAddr, buffer, &size);
  if (status != MFRC522::STATUS_OK) {
    Serial.print(F("MIFARE_Read() failed: "));
    Serial.println(mfrc522.GetStatusCodeName(status));
    Serial.println("\nFalha na leitura do saldo. Repita o procedimento\n");
    return;
  }

  String conteudo = "";
  byte letra;
  for (byte i = 0; i < mfrc522.uid.size; i++)
  {
    conteudo.concat(String(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " "));
    conteudo.concat(String(mfrc522.uid.uidByte[i], HEX));
  }
  conteudo.toUpperCase();
  CartaoAtual = conteudo;

  //Confere se o programa não está lendo o mesmo cartão novamente
  if ( CartaoAtual == CartaoAnterior ) {
    return;
  }
  if (conteudo.substring(1) == UidCartaoMestre) //Cartão mestre - Cobrador
  {
    Serial.println("Ola Cobrador !");
    Serial.println();
    Serial.println("Aproxime o cartao do usuario para carrega-lo com R$ 10,00.");
    Serial.println();
    modoRecarga = true;
  }

  if ((conteudo.substring(1) == UidUsuario1) && !modoRecarga)     // Usuario 1
  {
    printaSaldo("Ola usuario, seu saldo é: R$ ");

    bool saldoSuficiente = cobraPassagem();

    if (saldoSuficiente) {
      Serial.println("Acesso liberado!\n");
      printaSaldo("Seu saldo agora é: R$ ");

      granted();                                                  // Acende o LED verde por 2 segundos
    } else {
      Serial.println("Saldo insuficiente.\n");
      denied();                                                   // Acende o LED vermelho por 2 segundos
    }
  }

  if ((conteudo.substring(1) == UidUsuario2) && !modoRecarga)     // Usuario 2
  {
    Serial.print(F("Ola usuario, seu saldo é: R$ "));
    Serial.print(SaldoUsuario2);
    Serial.println("\n");

    if (SaldoUsuario2 >= 4.05) {
      Serial.println("Acesso liberado!\n");
      SaldoUsuario2 = SaldoUsuario2 - 4.05;
      Serial.print(F("Seu saldo agora é: R$ "));
      Serial.print(SaldoUsuario2);
      Serial.println("\n");
    } else {
      Serial.println("Saldo insuficiente.\n");
    }
  }

  if ((conteudo.substring(1) == UidUsuario1) && modoRecarga)     //Usuario 1 -- Modo Recarga
  {
    denied();

    dataBlock[0] = buffer[0];
    dataBlock[1] = buffer[1];

    printaSaldo("Ola usuario, seu saldo é: R$ ");

    dataBlock[0] = dataBlock[0] + 10;

    modoRecarga   = false;
    printaSaldo("Saldo atualizado: R$ ");

    // Write data to the block
    status = (MFRC522::StatusCode) mfrc522.MIFARE_Write(blockAddr, dataBlock, 16);
    if (status != MFRC522::STATUS_OK) {
      Serial.print(F("MIFARE_Write() failed: "));
      Serial.println(mfrc522.GetStatusCodeName(status));
      Serial.println("\nFalha na escrita do novo saldo. Repita o procedimento\n");
    }
    Serial.println();

    granted();
  }

  if ((conteudo.substring(1) == UidUsuario2) && modoRecarga)     //Usuario 2 -- Modo Recarga
  {
    Serial.print(F("Ola usuario, seu saldo é: R$ "));
    Serial.print(SaldoUsuario2);
    Serial.println("\n");

    SaldoUsuario2 = SaldoUsuario2 + 10.0;
    modoRecarga   = false;
    Serial.print(F("Saldo atualizado: R$ "));
    Serial.print(SaldoUsuario2);
    Serial.println("\n");
  }

  //CartaoAnterior = CartaoAtual;
  delay(3000);

  // Halt PICC
  mfrc522.PICC_HaltA();
  // Stop encryption on PCD
  mfrc522.PCD_StopCrypto1();
}


void carregaCartao(String UidUsuario)
{
  if (UidUsuario == UidUsuario1) {
    SaldoUsuario1 = SaldoUsuario1 + valorRecarga;
    Serial.print(F("Saldo atualizado no cartao: R$ "));
    Serial.print(SaldoUsuario1);
    Serial.println();
  }

  if (UidUsuario == UidUsuario2) {
    SaldoUsuario2 = SaldoUsuario2 + valorRecarga;
    Serial.print(F("Saldo atualizado no cartao: R$ "));
    Serial.print(SaldoUsuario2);
    Serial.println();
  }
}

void printaSaldo(String mensagem) {
  Serial.print(mensagem);
  Serial.print(dataBlock[0]); Serial.print("."); Serial.print(dataBlock[1]);
  Serial.println("\n");
  return;
}

bool cobraPassagem() {
  if (dataBlock[0] < 4) {                                                             // Menos do que 4 reais de saldo
    return false;
  }
  else if (dataBlock[0] > 4) {                                                        // Mais do que 4 reais de saldo
    if (dataBlock[1] >= 5) {                                                          
      dataBlock[0] = dataBlock[0] - 4;
      dataBlock[1] = dataBlock[1] - 5;
      return true;
    } else {
      dataBlock[0] = dataBlock[0] - 5;
      dataBlock[1] = dataBlock[1] + 95;
      return true;
    }
  }
  else {                                                                              // Exatamente 4 reais de saldo
    if (dataBlock[1] < 5) {
      return false;
    } else {
      dataBlock[0] = dataBlock[0] - 5;
      dataBlock[1] = dataBlock[1] + 95;
      return true;
    }
  }
}

/**
   Helper routine to dump a byte array as hex values to Serial.
*/
void dump_byte_array(byte *buffer, byte bufferSize) {
  for (byte i = 0; i < bufferSize; i++) {
    Serial.print(buffer[i] < 0x10 ? " 0" : " ");
    Serial.print(buffer[i], HEX);
  }
}

/////////////////////////////////////////  Access Granted    ///////////////////////////////////
void granted () {
  digitalWrite(redLed, LED_OFF);                             // Turn off red LED
  digitalWrite(greenLed, LED_ON);                            // Turn on green LED
  delay(2000);                                               // Hold green LED on for a second
  digitalWrite(greenLed, LED_OFF);
}

///////////////////////////////////////// Access Denied  ///////////////////////////////////
void denied() {
  digitalWrite(greenLed, LED_OFF);  // Make sure green LED is off
  digitalWrite(redLed  , LED_ON);   // Turn on red LED
  delay(2000);
  digitalWrite(redLed  , LED_OFF);
}
