#include <Wire.h>


int contador1 = 0;
int contador2 = 0;
int contador3 = 0;
int esperaLed = 2000; //tempo necessario para a mudança de estado
int led[3]; //pinos dos leds
int ledPrincipalAnterior; //Led definido pela posiçao da luminaria no ultimo loop
int ledPrincipalAtual;  //Led definido pela posição da luminaria no loop atual
int numLeds = 3;  //Numero de fitas de led
int pwm[3]; //Valores lidos pelo infravermelho
int pwmMax[3];  //Valores lidos pelo infravermelho na inicialização do led principal
int pwmMin = 15; //Menor valor para o led ligar
int pwmReferencia;  //Luminosidade atribuida na dimerização

bool estadoLed[3];  //Aramzena o estado (ligado ou desligado da fita de led)
bool fixo;  //Verifica de a luminosidade está fixada pela dimerização

const int MPU_ADDR = 0x68; // Endereço I2C do MPU-6050. Se o pini AD0 for ativado (HIGH) o endereço será 0x69.

int16_t acelerometro_x, acelerometro_y, acelerometro_z; //Valores lidos pelo acelerometro
char tmp_str[7]; // variavel usada na função de conversão


//============================================================================================================


void setup() {
  contador1 = 0;
  contador2 = 0;
  fixo = false;
  led[0] = 9;
  led[1] = 10;
  led[2] = 6;
  
  Serial.begin(9600);
  pinMode(led[0], OUTPUT);
  pinMode(led[1], OUTPUT);
  pinMode(led[2], OUTPUT);
  digitalWrite(led[0], LOW);
  digitalWrite(led[1], LOW);
  digitalWrite(led[2], LOW);
  Wire.begin();
  Wire.beginTransmission(MPU_ADDR); // Inicia a transmissão com o I2C slave (GY-521 board)
  Wire.write(0x6B); // PWR_MGMT_1 registra
  Wire.write(0); // seta para zero (aciona a MPU-6050)
  Wire.endTransmission(true); 
  
  lerGiroscopio();
  ledPrincipal();
  iniciaLedPrincipal();
}


//============================================================================================================


void loop() {
  lerGiroscopio();
  ledPrincipal();
  if (ledPrincipalAtual != ledPrincipalAnterior) {
    iniciaLedPrincipal();
  }
  ledSecundario();
  dimerizar();
}


//============================================================================================================


char* convert_int16_to_str(int16_t i) { // converte int16 para string.
  sprintf(tmp_str, "%6d", i);
  return tmp_str;
}


void lerInfraVermelho() {
  pwm[0] = map(analogRead(A2), 0, 1023, 0, 255); //Normaliza os valores de 0 a 1023 (infravermelho) para 0 a 255 (voltagem led);
  pwm[1] = map(analogRead(A0), 0, 1023, 0, 255);
  pwm[2] = map(analogRead(A7), 0, 1023, 0, 255);
}


void iniciaComunicacao() {
  Wire.beginTransmission(MPU_ADDR); //inicia transmissao
  Wire.write(0x3B); //start com o registro 0x3B (ACCEL_XOUT_H) [MPU-6000 and MPU-6050 Register Map and Descriptions Revision 4.2, p.40]
  Wire.endTransmission(false);  // o parâmetro indica que o Arduino enviará uma reinicialização. Como resultado, a conexão é mantida ativa
  Wire.requestFrom(MPU_ADDR, 7*2, true);  // solicita um total de 7 * 2 = 14 registros
}


void lerGiroscopio(){
  iniciaComunicacao();
  // "Wire.read()<<8 | Wire.read();" Dois registros são lidos e armazenados na mesma variável
  acelerometro_x = Wire.read()<<8 | Wire.read(); // lê o registro: 0x3B (ACCEL_XOUT_H) e 0x3C (ACCEL_XOUT_L)
  acelerometro_y = Wire.read()<<8 | Wire.read(); // lê o registro: 0x3D (ACCEL_YOUT_H) e 0x3E (ACCEL_YOUT_L)
  acelerometro_z = Wire.read()<<8 | Wire.read(); // lê o registro: 0x3F (ACCEL_ZOUT_H) e 0x40 (ACCEL_ZOUT_L)
  acelerometro_x = abs(acelerometro_x); 
  acelerometro_y = abs(acelerometro_y);
  acelerometro_z = abs(acelerometro_z);
   
  //print out data
  //Serial.print("aX = "); Serial.print(convert_int16_to_str(acelerometro_x));
  //Serial.print(" | aY = "); Serial.print(convert_int16_to_str(acelerometro_y));
  //Serial.print(" | aZ = "); Serial.print(convert_int16_to_str(acelerometro_z));
  //Serial.println();
}


void ledPrincipal(){
  //define o led principal
  if (acelerometro_x > acelerometro_y && acelerometro_x > acelerometro_z){
    ledPrincipalAtual = 0;
  }  
  if (acelerometro_y > acelerometro_z && acelerometro_y > acelerometro_x){
    ledPrincipalAtual = 1;
  }
   if (acelerometro_z > acelerometro_x && acelerometro_z > acelerometro_y){
    ledPrincipalAtual = 2;
  }
}


void iniciaLedPrincipal(){
  //Atribui o estado dos leds após a rotação da luminaria
  for (int i=0; i<numLeds; i++) {
    if (i == ledPrincipalAtual) {
      estadoLed[i] = true;
    }
    else {
      estadoLed[i] = false;
      analogWrite(led[i], 0); //Desliga os leds secundários
    }
  }
  //Acende o led principal
  for (int i=0; i<=255; i++){
    analogWrite(led[ledPrincipalAtual], i);
    delay(10);
  }
  //Atribui os valores máximos lidos nos sensores infravermelho
  lerInfraVermelho();
  for (int i = 0; i < numLeds; i++){
    pwmMax[i] = pwm[i];
  }
  ledPrincipalAnterior = ledPrincipalAtual; //Atualiza o led principal atual
}


void ledSecundario(){
  for (int i=0; i<numLeds; i++) {
    if (!estadoLed[i]){
      while(pwm[i]<30){
        lerInfraVermelho();
        contador3 = contador3 + 1;
        if (contador3 > esperaLed/6) {
          estadoLed[i] = true;
          analogWrite(led[i], 50);
        }
      }
      contador3 = 0;
    }
  }
}



void dimerizar() {
  lerInfraVermelho();
  for (int i=0; i<numLeds; i++) {
    if (estadoLed[i]){  //Verifica se o led está ativo.  
      if (pwmMax[i] - pwm[i]>30) {  //Verifica se existe algum objeto dentro do alcance do infravermelho.
        fixo = false; //Habilita a variação de tensão no led.
        pwmReferencia = pwm[i]; //Atribui um valor de referencia para a nova luminosidade.
        while (abs(pwm[i] - pwmReferencia) < 15){ // Verifica se a posição para a nova luminosidade é mantida 
          //Serial.print("pwm: "); Serial.print(pwm[i]);
          //Serial.println();
          lerInfraVermelho();
          //Serial.print("pwm: "); Serial.print(pwm[i]);
          //Serial.println();
          contador1 = contador1 + 1;
          if (pwm[i]<=pwmMin){
            analogWrite(led[i], 0);
          }
          else{
            analogWrite(led[i], pwm[i]);
          }
          if (contador1 > esperaLed) {  //Verifica a posição pelo tempo necessário à mudança de luminosidade.
            fixo = true; //Desabilita variação de tensão no led.
            piscar(led[i], pwmReferencia);  //Pisca para confirmar a nova luminosidade.
            if (pwm[i]<=pwmMin){
            analogWrite(led[i], 0);
            estadoLed[i] = false;
            }
            while(pwmMax[i]-pwm[i]>15) {  //Fixa a luminosidade até a retirada da mão.
              lerInfraVermelho();
              delay(50);
            }
          }
        }
        contador1 = 0;
        contador2 = 0;
        if (pwmReferencia<=30){
            pwmReferencia = 0;
        }
        analogWrite(led[i], pwmReferencia);
      }
      if (!fixo) { //Se o led estiver ligado e sem voltagem fixa, atribui voltagem máxima para o led.
        contador2 = contador2 + 1;  //Temporizador para melhorar usabilidade.
        if (contador2 >= 10){ //Temporizador para melhorar usabilidade.
          analogWrite(led[i], 255);  
        }
      }
    }
  }
}


//Pisca para confirmar que a luminosidade foi fixada.
void piscar(int led, int intensidade) {
  for (int i=0; i<=intensidade; i++){
    analogWrite(led, (intensidade - i));
    delay(150/intensidade);
  }
  delay(50);
  for (int i=0; i<=intensidade; i++){
    analogWrite(led, i);
    delay(150/intensidade);    
  }
}
