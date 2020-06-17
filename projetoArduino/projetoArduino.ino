
#include <SoftwareSerial.h> //INCLUSÃO DE BIBLIOTECA
#include <MFRC522.h>
#include <LiquidCrystal_I2C.h>
#include <Keypad.h>
#include <Servo.h>
#include <SPI.h>

const int rxPin = 3;
const int txPin = 4;
const int buzzerPin = 5;
const int redLed = 6;
const int greenLed = 7;
const int servoPin = 8;

String receivedData = "";
char initial_password[4] = {'1', '2', '4', '5'};  // Armazena a senha inicial em um array
char password[4];  // Armazena a senha inserida pelo usuário
char key_pressed = 0;  // Armazena os botões
String tagUID = "49 C5 FA 97";  // Armazena o ID da tag usada pelo RFID
bool RFIDMode = true;
String mode = "0";
int i = 0;  // Contador das teclas pressionadas

// Define número de linhas e colunas do teclado matricial
const int rows = 4;
const int columns = 4;
// Mapeia o layout do teclado
char hexaKeys[rows][columns] = {
  {'1', '2', '3', 'A'},
  {'4', '5', '6', 'B'},
  {'7', '8', '9', 'C'},
  {'*', '0', '#', 'D'}
};
// Pins do teclado
byte row_pins[rows] = {A0, A1, A2, A3};
byte column_pins[columns] = {0, 1, 2};

Keypad keypad_key = Keypad( makeKeymap(hexaKeys), row_pins, column_pins, rows, columns); // Teclado matricial
LiquidCrystal_I2C lcd(0x27, 2, 1, 0, 4, 5, 6, 7, 3, POSITIVE); // Painel LCD
SoftwareSerial bluetooth(rxPin, txPin);  // Módulo bluetooth
MFRC522 mfrc522(10, 9); // Leitor RFID(SS_PIN, RST_PIN)
Servo sg90; // Servo motor



void setup() {
  bluetooth.begin(9600);
  pinMode(buzzerPin, OUTPUT);
  pinMode(redLed, OUTPUT);
  pinMode(greenLed, OUTPUT);
  pinMode(4, OUTPUT);
  sg90.attach(servoPin);
  sg90.write(0); // Posição inicial do servo motor
  lcd.begin(16, 2);  // Inicia a tela LCD de 16x2
  lcd.backlight();   // Luz de fundo
  SPI.begin();
  mfrc522.PCD_Init();   // Inicia o RFID
  lcd.clear(); // Limpa o display LCD
}

void loop() {

  if (mode == "0") {
    if (RFIDMode == true) {
      read_data(); //Função para receber dados via bluetooth
      lcd.setCursor(0, 0);
      lcd.print("  Door Closed   ");
      lcd.setCursor(0, 1);
      lcd.print(" Scan your tag  ");;
      // Procura novas tags
      if ( ! mfrc522.PICC_IsNewCardPresent()) {
        return;
      }
      // Seleciona uma tag
      if ( ! mfrc522.PICC_ReadCardSerial()) {
        return;
      }
      // Leitura da tag
      String tag = "";
      for (byte j = 0; j < mfrc522.uid.size; j++) {
        tag.concat(String(mfrc522.uid.uidByte[j] < 0x10 ? " 0" : " "));
        tag.concat(String(mfrc522.uid.uidByte[j], HEX));
      }
      tag.toUpperCase();
      // Verificando a tag
      if (tag.substring(1) == tagUID) { // Se o ID da tag utilizada for igual ao da tag permitida
        lcd.clear();
        lcd.print("Tag Accepted    ");
        delay(500);
        lcd.clear();
        lcd.print("Enter Password: ");
        lcd.setCursor(0, 1);
        RFIDMode = false; // Desativa a leitura de tags
      }
      else { // Se o ID da tag não for igual ao da tag permitida
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Wrong Tag!      ");
        lcd.setCursor(0, 1);
        lcd.print("Access Denied!  ");
        tone(buzzerPin, 65);
        digitalWrite(redLed, HIGH);
        delay(300);
        noTone(buzzerPin);
        digitalWrite(redLed, LOW);
        delay(100);
        tone(buzzerPin, 65);
        digitalWrite(redLed, HIGH);
        delay(1000);
        noTone(buzzerPin);
        digitalWrite(redLed, LOW);
        lcd.clear();
      }
    }

    // Se o modo RFID for desativado, o sistema ficará em busca de uma senha
    if (RFIDMode == false) {
      key_pressed = keypad_key.getKey(); // Método de armazenar botões
      if (key_pressed) {
        password[i++] = key_pressed; // Armazenando na variável que armazena botões
        lcd.print("*");
      }
      if (i == 4) { // se 4 teclas forem pressionadas
        delay(200);
        if (!(strncmp(password, initial_password, 4))) { // Se a senha estiver certa
          lcd.clear();
          lcd.setCursor(0, 0);
          lcd.print("    Password    ");
          lcd.setCursor(0, 1);
          lcd.print("    Accepted    ");
          sg90.write(120); // Porta Aberta
          opening_sound();
          sg90.write(0); // Porta Fechada
          lcd.clear();
          i = 0;
          RFIDMode = true; // Ativa o modo RFID
        }
        else { // Se a senha estiver errada
          lcd.clear();
          lcd.print(" Wrong Password ");
          tone(buzzerPin, 65);
          digitalWrite(redLed, HIGH);
          delay(300);
          noTone(buzzerPin);
          digitalWrite(redLed, LOW);
          delay(100);
          tone(buzzerPin, 65);
          digitalWrite(redLed, HIGH);
          delay(1000);
          noTone(buzzerPin);
          digitalWrite(redLed, LOW);
          lcd.clear();
          i = 0;
          RFIDMode = true;  // Ativa o modo RFID
        }
      }
    }
  }

  if (mode == "1") {
    lcd.setCursor(0, 0);
    lcd.print("  Door  is Open ");
    lcd.setCursor(0, 1);
    lcd.print("                ");
    read_data();
  }

  if (mode == "2") {
    lcd.setCursor(0, 0);
    lcd.print("     System     ");
    lcd.setCursor(0, 1);
    lcd.print("     Locked     ");
    read_data();
  }

  receivedData = ""; //Limpa a mensagem recebida
}

void read_data() {
  while (bluetooth.available()) {
    char rxx = bluetooth.read();
    receivedData.concat(rxx);   // Cria a mensagem lida pelo arduino, concatenando os dados recebidos
    delay(10);
    if (receivedData == "1" && mode != "2") {     //Se receber 1, abre a porta
      bluetooth.println("OPENED");
      sg90.write(120); // Porta Aberta
      lcd.clear();
      lcd.print("  Opening Door  ");
      opening_sound();
      delay(300);
      lcd.clear();
      mode = "1";
    }
    if (receivedData == "2" && mode == "1") {     //Se receber 2, fecha a porta
      bluetooth.println("CLOSED");
      sg90.write(0); // Porta Fechada
      mode = "0";
    }
    if (receivedData == "3" && mode == "0") {   //Trava o sistema, impedindo qualquer ação de abrir a porta
      bluetooth.println("LOCKED");
      mode = "2";
    }
    if (receivedData == "4" && mode == "2") {    //Destrava o sistema, o sistema volta a funcionar normalmente
      bluetooth.println("UNLOCKED");
      lcd.clear();
      lcd.print("     System     ");
      lcd.setCursor(0, 1);
      lcd.print("    Unlocked    ");
      opening_sound();
      mode = "0";
    }
    receivedData = "";
  }
}

void opening_sound() {
  for (int y = 0; y < 2; y++) {
    digitalWrite(greenLed, HIGH);
    tone(buzzerPin, 988);
    delay(700);
    digitalWrite(greenLed, LOW);
    noTone(buzzerPin);
    delay(300);
  }
}
