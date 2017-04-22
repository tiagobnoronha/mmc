#include "DHT.h" // biblioteca DHT11
#include "LiquidCrystal.h" // biblioteca para usar o display
#include "Limits.h"
#include "SPI.h"
#include "SD.h"
#include "Wire.h"
#include <Ethernet.h>

#define DHTPIN  A2 // pino que estamos conectado
#define DHTTYPE DHT11
#define DS1307_ADDRESS 0x68

DHT dht(DHTPIN, DHTTYPE);

byte zero = 0x00;

// endereço mac do módulo
byte mac[] = {
  0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED
};

const int sensorLuz  = A0; //Pino analógico em que o sensor de luz está conectado.
const int sensorTemp = A1; //Pino analógico em que o sensor de temperatura no sol está conectado.
const int chipSelect = 7; //CS módulo SD

int valorSensorLuz  = 0;  //variável usada para ler o valor do sensor de luz.
int valorSensorTemp = 0;  //variável usada para ler o valor do sensor de temperatura.

long tempoAnteriorTemp = 0;
long tempoAnteriorLux = 0;
long tempoAnteriorUmid = 0;

int t = 1;
int l = 1;
int u = 1;

String hora = "";

char c;

LiquidCrystal lcd(9, 8, 6, 5, 4, 3, 2);

byte sol[8] = {
  B00100,
  B10001,
  B01110,
  B10001,
  B10001,
  B01110,
  B10001,
  B00100
};

byte nuvem[8] = {
  B00000,
  B00000,
  B01110,
  B11011,
  B11011,
  B01110,
  B00000,
  B00000
};

EthernetServer server(80);

void Mostrarelogio();

void setup() {
  //Inicializando o LCD e informando o tamanho de 16 colunas e 2 linhas
  Serial.begin(115200); // inicializa a serial
  while (!Serial) {
    ; // espera pela porta serial
  }
  dht.begin();
  Wire.begin();

  lcd.createChar(0, sol); // cria o desenho do sol no display
  lcd.createChar(1, nuvem); // cria o desenho da nuvem no display

  lcd.begin(16, 2); // inicializa o display
  lcd.setCursor(6, 0);
  lcd.print("Data");
  lcd.setCursor(0, 1);
  lcd.write(byte(1));
  lcd.setCursor(5, 1);
  lcd.print("Logger");
  lcd.setCursor(15, 1);
  lcd.write(byte(0));

  Serial.println("\t \t \t Data Logger Autossustentavel de Baixo Custo");
  Serial.print("Initializing SD card...");

  // verifica se o SD Card está presente
  if (!SD.begin(chipSelect)) {
    Serial.println("Falha ao encontrar SD Card");

    //return;
  }
  Serial.println("Cartão encontrado");

  if (Ethernet.begin(mac) == 0) {
    Serial.println("Falha em conectar à internet");
  }
  server.begin();
  Serial.print("Servidor está em: ");
  Serial.println(Ethernet.localIP());

  delay(3500);
}


void loop() {
  String dataStringTemp = "";
  String dataStringLux = "";
  String dataStringUmid = "";

  // leitura sensor temperatura
  int mediaValorTemp  = 0;  //Variável usada para armazenar o menor valor da temperatura.
  int somaValorTemp = 0;
  int temp;

  somaValorTemp = 0;
  temp = 0;
  for (int k = 1; k <= 8; k++) {
    //Lendo o valor do sensor de temperatura.
    temp = analogRead(sensorTemp);

    somaValorTemp += temp;

    delay(250);
  }

  mediaValorTemp = somaValorTemp / 8;
  //Transformando valor lido no sensor de temperatura em graus celsius aproximados.
  mediaValorTemp = (500 * mediaValorTemp) / 1023;

  valorSensorTemp = mediaValorTemp;

  Serial.print("Temperatura: ");
  Serial.println(valorSensorTemp);

  dataStringTemp += String(valorSensorTemp);

  //Exibindo valor da leitura do sensor de temperatura no display LCD.
  lcd.clear();  //limpa o display do LCD.
  lcd.setCursor(2, 0);
  lcd.print("Temperatura: ");  //imprime a string no display do LCD.
  lcd.setCursor(6, 1);
  lcd.print(valorSensorTemp);
  lcd.write(B11011111); //Simbolo de graus celsius
  lcd.print("C");

  delay(1000);

  // Sensor de luz
  int mediaValorLuz  = 0;
  int somaValorLuz = 0;
  int luz = 0;

  for (int i = 1; i <= 8; i++) {
    //Lendo o valor do sensor de luz
    luz = analogRead(sensorLuz);

    somaValorLuz += luz;

    delay(250);
  }

  mediaValorLuz = somaValorLuz / 8;

  valorSensorLuz  = mediaValorLuz;

  double lux = pow((9.78 * valorSensorLuz) / (1E6 - 978.0 * valorSensorLuz), 5.0 / 3);

  //Limpa o display
  lcd.clear();
  lcd.setCursor(2, 0);
  lcd.print("Luminosidade: ");  //imprime a string no display do LCD.
  lcd.setCursor(3, 1);
  lcd.print(lux, 4);
  lcd.setCursor(10, 1);
  lcd.print("Lux");

  Serial.print("Luminosidade: ");
  Serial.print(lux, 4);
  Serial.println(" Lux");

  dataStringLux += String(lux, 4);

  delay(1000);

  // Sensor de humidade
  int mediaValorHumidade  = 0;
  int somaValorHumidade = 0;
  int humidade = 0;

  for (int j = 1; j <= 8; j++) {
    //Lendo o valor do sensor de humidade
    humidade = dht.readHumidity();

    somaValorHumidade += humidade;

    delay(250);
  }

  mediaValorHumidade = somaValorHumidade / 8;

  float h = mediaValorHumidade;

  lcd.clear();

  if (isnan(h))
  {
    Serial.println("Falha ao achar DHT11");
  }
  else
  {
    Serial.print("Umidade: ");
    Serial.print(h);
    Serial.println(" % ");

    dataStringUmid += String(h);

    lcd.setCursor(3, 0);
    lcd.print("Umidade do");
    lcd.setCursor(0, 1);
    lcd.print("Ambiente: ");
    lcd.print(h, 2);
    lcd.print(" % ");
    delay(1000);
  }

  if (millis() - tempoAnteriorTemp > 10000) {
    tempoAnteriorTemp = millis();

    File dataFile1 = SD.open("temp.txt", FILE_WRITE); // Abre o arquivo temp.txt no SD Card

    // verifica se o arquivo esta disponivel
    if (dataFile1) {
      dataFile1.print(dataStringTemp);
      dataFile1.print(";");
      dataFile1.println(hora);
      dataFile1.close();

      Serial.print(dataStringTemp);
      Serial.println("");
    }
    // printa mensagem de erro
    else {
      Serial.println("Erro ao abrir temp.txt");
    }
  }

  if (millis() - tempoAnteriorLux > 10000) {
    tempoAnteriorLux = millis();

    File dataFile2 = SD.open("lux.txt", FILE_WRITE); // Abre o arquivo lux.txt no SD Card

    // verifica se o arquivo esta disponivel
    if (dataFile2) {
      dataFile2.print(dataStringLux);
      dataFile2.print(";");
      dataFile2.println(hora);
      dataFile2.close();

      Serial.print(dataStringLux);
      Serial.println("");
    }
    // printa mensagem de erro
    else {
      Serial.println("Erro ao abrir lux.txt");
    }
  }

  if (millis() - tempoAnteriorUmid > 10000) {
    tempoAnteriorUmid = millis();

    File dataFile3 = SD.open("umidade.txt", FILE_WRITE); // Abre o arquivo umidade.txt no SD Card

    // verifica se o arquivo esta disponivel
    if (dataFile3) {
      dataFile3.print(dataStringUmid);
      dataFile3.print(";");
      dataFile3.println(hora);
      dataFile3.close();

      Serial.print(dataStringUmid);
      Serial.println("");
    }
    // printa mensagem de erro
    else {
      Serial.println("Erro ao abrir umid.txt");
    }
  }
  Mostrarelogio();
  Serial.println(hora);

  EthernetClient client = server.available();
  if (client) {
    Serial.println("Novo cliente");
    // um http request acaba com um blank line
    boolean currentLineIsBlank = true;
    while (client.connected()) {
      if (client.available()) {
        c = client.read();
        Serial.write(c);
        // se chegou ao fim da linha (recebeu um caractere de nova linha)
        // e a linha estiver em branco, o pedido http terminou,
        // assim pode enviar uma resposta
        if (c == '\n' && currentLineIsBlank) {
          // enviar um cabeçalho de resposta HTTP padrão
          client.println("HTTP/1.1 200 OK");
          client.println("Content-Type: application/json");
          client.println("Connection: close");  // a conecçao vai ser fechada depois da conclusão da resposta
          client.println();
          client.print("{luminosidade:");
          client.print(lux,4);
          client.print("}");
          break;
        }
      }
      if (c == '\n') {
        // you're starting a new line
        currentLineIsBlank = true;
      } else if (c != '\r') {
        // you've gotten a character on the current line
        currentLineIsBlank = false;
      }
    }
  }
  // give the web browser time to receive the data
  delay(1);
  // close the connection:
  client.stop();
  Serial.println("client disconnected");
}

byte ConverteParaBCD(byte val) { //Converte o número de decimal para BCD
  return ( (val / 10 * 16) + (val % 10) );
}

byte ConverteparaDecimal(byte val)  { //Converte de BCD para decimal
  return ( (val / 16 * 10) + (val % 16) );
}

void Mostrarelogio()
{
  Wire.beginTransmission(DS1307_ADDRESS);
  Wire.write(zero);
  Wire.endTransmission();
  Wire.requestFrom(DS1307_ADDRESS, 7);
  int segundos = ConverteparaDecimal(Wire.read());
  int minutos = ConverteparaDecimal(Wire.read());
  int horas = ConverteparaDecimal(Wire.read() & 0b111111);
  int diadasemana = ConverteparaDecimal(Wire.read());
  int diadomes = ConverteparaDecimal(Wire.read());
  int mes = ConverteparaDecimal(Wire.read());
  int ano = ConverteparaDecimal(Wire.read());
  //Mostra a data no Serial Monitor

  hora =  String();
  hora += String(diadomes);
  hora += String("/");
  hora += String(mes);
  hora += String("/");
  hora += String(ano);
  hora += String(";");
  hora += String(horas);
  hora += String(":");
  hora += String(minutos);
  hora += String(":");
  hora += String(segundos);
}
