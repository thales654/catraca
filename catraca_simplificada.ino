#include <SPI.h>
#include <MFRC522.h>
#include <LiquidCrystal.h>

#define SS_PIN  53
#define RST_PIN 5                               // Pino de Reset do MRF

MFRC522 mfrc522(SS_PIN, RST_PIN);               // Create MFRC522 instance.

const String Versao          = "v0.4";          // Versão do Firmware

const String UidCartaoMestre = "2D C3 76 89";   // UID do Cartao Mestre

const String UidUsuario1     = "EA 30 B2 73";   // UID do Usuario 1
const String UidUsuario2     = "F5 94 BF 65";   // UID do Usuario 2

String CartaoAtual           = "00 00 00 00";
String CartaoAnterior        = "FF FF FF FF";

float SaldoUsuario1 = 0;
float SaldoUsuario2 = 0;

const float valorRecarga     = 10.00;           // Valor da recarga

bool modoRecarga             = false;

MFRC522::MIFARE_Key key;

void setup()
{
  Serial.begin(9600);                           // Inicia a serial
  SPI.begin();                                  // Inicia  SPI bus
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

  if ((conteudo.substring(1) == UidUsuario1) && !modoRecarga)     //Usuario 1
  {
    MFRC522::StatusCode status;

    // In this sample we use the second sector,
    // that is: sector #1, covering block #4 up to and including block #7
    byte sector         = 0;
    byte blockAddr      = 0;
    byte dataBlock[]    = {
      0x04, 0x05, 0xFF, 0xFF, //  4 ,  5 , 255, 255,
      0xFF, 0xFF, 0xFF, 0xFF, // 255, 255, 255, 255,
      0xFF, 0xFF, 0xFF, 0xFF, // 255, 255, 255, 255,
      0xFF, 0xFF, 0xFF, 0xFF  // 255, 255, 255, 255
    };
    byte trailerBlock   = 4;

    // Authenticate using key A
    Serial.println(F("Authenticating using key A..."));
    status = (MFRC522::StatusCode) mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, trailerBlock, &key, &(mfrc522.uid));
    if (status != MFRC522::STATUS_OK) {
      Serial.print(F("PCD_Authenticate() failed: "));
      Serial.println(mfrc522.GetStatusCodeName(status));
      return;
    }

    // Show the whole sector as it currently is
    Serial.println(F("Current data in sector:"));
    mfrc522.PICC_DumpMifareClassicSectorToSerial(&(mfrc522.uid), &key, sector);
    Serial.println();

    Serial.print(F("Ola usuario, seu saldo é: R$ "));
    Serial.print(SaldoUsuario1);
    Serial.println("\n");

    if (SaldoUsuario1 >= 4.05) {
      Serial.println("Acesso liberado!\n");
      SaldoUsuario1 = SaldoUsuario1 - 4.05;
      Serial.print(F("Seu saldo agora é: R$ "));
      Serial.print(SaldoUsuario1);
      Serial.println("\n");
    } else {
      Serial.println("Saldo insuficiente.\n");
    }
  }

  if ((conteudo.substring(1) == UidUsuario2) && !modoRecarga)     //Usuario 2
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
    Serial.print(F("Ola usuario, seu saldo é: R$ "));
    Serial.print(SaldoUsuario1);
    Serial.println("\n");

    SaldoUsuario1 = SaldoUsuario1 + 10.0;
    modoRecarga   = false;
    Serial.print(F("Saldo atualizado: R$ "));
    Serial.print(SaldoUsuario1);
    Serial.println("\n");
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
