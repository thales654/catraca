#include <SPI.h>
#include <MFRC522.h>
#include <LiquidCrystal.h>

#define SS_PIN  53
#define RST_PIN 5

MFRC522 mfrc522(SS_PIN, RST_PIN);  // Create MFRC522 instance.

const String Versao          = "v0.3";

const String UidCartaoMestre = "2D C3 76 89";

const String UidUsuario1     = "EA 30 B2 73";
const String UidUsuario2     = "F5 94 BF 65";

String CartaoAtual           = "00 00 00 00";
String CartaoAnterior        = "FF FF FF FF";

float SaldoUsuario1 = 0;
float SaldoUsuario2 = 0;

float valorRecarga  = 10.00;

bool modoRecarga    = false;

void setup() 
{
  Serial.begin(9600); // Inicia a serial
  SPI.begin();    // Inicia  SPI bus
  mfrc522.PCD_Init(); // Inicia MFRC522
  Serial.print("Sistema de Catraca ");
  Serial.print(Versao);
  Serial.println("\n");
  Serial.println("Aproxime o seu cartao do leitor...");
  Serial.println();
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
  
  String conteudo= "";
  byte letra;
  for (byte i = 0; i < mfrc522.uid.size; i++) 
  {
     conteudo.concat(String(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " "));
     conteudo.concat(String(mfrc522.uid.uidByte[i], HEX));
  }
  conteudo.toUpperCase();
  CartaoAtual = conteudo;
  
  //Confere se o programa não está lendo o mesmo cartão novamente
  if ( CartaoAtual == CartaoAnterior ){
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
    Serial.print(F("Ola usuario, seu saldo é: R$ "));
    Serial.print(SaldoUsuario1);
    Serial.println("\n");

    if (SaldoUsuario1 >= 4.05){
      Serial.println("Acesso liberado!\n");
      SaldoUsuario1 = SaldoUsuario1 - 4.05;      
      Serial.print(F("Seu saldo agora é: R$ "));
      Serial.print(SaldoUsuario1);
      Serial.println("\n");
      delay(3000);
      return;
    }else{
      Serial.println("Saldo insuficiente.\n");
    }

    delay(3000);
  }

  if ((conteudo.substring(1) == UidUsuario2) && !modoRecarga)     //Usuario 2
  {
    Serial.print(F("Ola usuario, seu saldo é: R$ "));
    Serial.print(SaldoUsuario2);
    Serial.println("\n");

    if (SaldoUsuario2 >= 4.05){
      Serial.println("Acesso liberado!\n");
      SaldoUsuario2 = SaldoUsuario2 - 4.05;
      Serial.print(F("Seu saldo agora é: R$ "));
      Serial.print(SaldoUsuario2);
      Serial.println("\n");
      delay(3000);
      return;
    }else{
      Serial.println("Saldo insuficiente.\n");
    }

    delay(3000);
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

    delay(3000);
    return;
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

    delay(3000);
    return;
  }

  CartaoAnterior = CartaoAtual;  
}


void carregaCartao(String UidUsuario)
{
  if(UidUsuario == UidUsuario1){
    SaldoUsuario1 = SaldoUsuario1 + valorRecarga;
    Serial.print(F("Saldo atualizado no cartao: R$ "));
    Serial.print(SaldoUsuario1);
    Serial.println();
  }

  if(UidUsuario == UidUsuario2){
    SaldoUsuario2 = SaldoUsuario2 + valorRecarga;
    Serial.print(F("Saldo atualizado no cartao: R$ "));
    Serial.print(SaldoUsuario2);
    Serial.println();
  }
}
