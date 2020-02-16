#include <SoftwareSerial.h>
#include <Wire.h> 
#include <LiquidCrystal_I2C.h> 

SoftwareSerial hm10(2, 3); //RX, TX 연결
LiquidCrystal_I2C lcd(0x27, 16, 2); //lcd주소값, 배열

#define TX_Power -61 //1[m]인경우 RSSI
#define Length 5 //넓이 반경
#define N 3

char b[2], c[2], d[2], e[2]; // rssi char형 저장
int b1, b2; // (0,0) rssi char형으로 저장
int c1, c2; // (Length,0) rssi char형으로 저장
int d1, d2; // (Length,Length) rssi char형으로 저장
int e1, e2; // (0,Length) rssi char형으로 저장
float b0, c0, d0, e0; // rssi float형으로 저장
float t1, t2, t3, t4; // rssi 거리로 변환해서 저장
float x1, x2, x3, x4, y1, y2, y3, y4; // 좌표저장
float x, y, X[100], Y[100]; // 평균좌표저장



void SendAT(String str) { //AT커맨드 자동입력, 시리얼모니터 출력
  Serial.println(str);
  hm10.println(str);
}

void PrintReq(unsigned long t) { //프린트리퀘스트 : 응답시간 설정
  unsigned long last = millis();
  while (millis() - last < t) {
    while (hm10.available() > 0) {
      Serial.write(hm10.read());
    }
  }
}

void PrintReq2(unsigned long t) { //프린트리퀘스트2 : 비콘검색시간, 비콘MacAddress판별, rssi 저장
  char a[16]; // 16바이트씩 잘라내기 위한 배열
  unsigned long last = millis();
  while (millis() - last < t) {
    while (hm10.available() > 0) {
      hm10.readBytesUntil('\n', a, 16);
      //Serial.println(a);


      if ((a[6] == '6') && (a[7] == '3') && (a[8] == 'C')) { //MacAdress 뒷 세자리로 (0,0)비콘 판별
        b[0] = a[12]; //Rssi 10의 자리 저장
        b[1] = a[13]; //Rssi 1의 자리 저장
      }
      if ((a[6] == '6') && (a[7] == '5') && (a[8] == '8')) { //MacAdress 뒷 세자리로 (Length,0)비콘 판별
        c[0] = a[12]; //Rssi 10의 자리 저장
        c[1] = a[13]; //Rssi 1의 자리 저장
      }
      if ((a[6] == 'A') && (a[7] == '0') && (a[8] == '3')) { //MacAdress 뒷 세자리로 (Length,Length)비콘 판별
        d[0] = a[12]; //Rssi 10의 자리 저장
        d[1] = a[13]; //Rssi 1의 자리 저장
      }
      if ((a[6] == '5') && (a[7] == '9') && (a[8] == '5')) { //MacAdress 뒷 세자리로 (0,Length)비콘 판별
        e[0] = a[12]; //Rssi 10의 자리 저장
        e[1] = a[13]; //Rssi 1의 자리 저장
      } 
    }
  }
}

void setup() {
  Serial.begin(9600);
  hm10.begin(9600);
  SendAT("AT"); // AT커맨드 응답확인
  PrintReq(1000);
  SendAT("AT+ROLE1"); // 모듈 MASTER 모드설정1
  PrintReq(1000);
  SendAT("AT+IMME1"); // 모듈 MASTER 모드설정2
  PrintReq(1000);
  lcd.init(); //lcd 초기화
  lcd.backlight(); //lcd 백라이트 킴
}

void loop() {

  SendAT("AT+DISI?"); // 주위 비콘 검색 16바이트 잘라서 출력
  PrintReq2(5000);

  // char형 배열을 int형으로 바꾸기
  b1 = (b[0] - 48) * 10; // 10의 자리
  b2 = b[1] - 48; // 1의 자리
  b0 = -(b1 + b2); // 2개 더하고 -붙여서 float 변수 하나에 저장

  c1 = (c[0] - 48) * 10; // 10의 자리
  c2 = c[1] - 48; // 1의 자리
  c0 = -(c1 + c2); // 2개 더하고 -붙여서 float 변수 하나에 저장

  d1 = (d[0] - 48) * 10; // 10의 자리
  d2 = d[1] - 48; // 1의 자리
  d0 = -(d1 + d2); // 2개 더하고 -붙여서 float 변수 하나에 저장

  e1 = (e[0] - 48) * 10; // 10의 자리
  e2 = e[1] - 48; // 1의 자리
  e0 = -(e1 + e2); // 2개 더하고 -붙여서 float 변수 하나에 저장

  t1 = pow(10, ((TX_Power - b0) / (10 * N))); // rssi 값을 거리[m]로 변환 (0,0)
  t2 = pow(10, ((TX_Power - c0) / (10 * N))); // rssi 값을 거리[m]로 변환 (Length,0)
  t3 = pow(10, ((TX_Power - d0) / (10 * N))); // rssi 값을 거리[m]로 변환 (Length,Length)
  t4 = pow(10, ((TX_Power - e0) / (10 * N))); // rssi 값을 거리[m]로 변환 (0,Length)
  //----------------------------------------------------------------------
  if (t1 + t2 > Length) { // 2개의 비콘의 반경이 겹칠경우
    x1 = Length - abs((t1 + t2) / 2); // x좌표 구하기
    y1 = t1 * sin(acos(x1 / t1)); // y좌표 구하기
  }
  else { // 2개의 비콘의 반경이 겹치지 않을 경우
    x1 = t1 + abs((Length - t1 - t2) / 2); // x좌표 구하기
    y1 = 0; // y좌표 구하기
  }
  //----------------------------------------------------------------------
  if (t2 + t3 > Length) { // 2개의 비콘의 반경이 겹칠경우
    y2 = abs((t2 - t3) / 2); // y좌표 구하기
    x2 = Length - t2 * sin(acos(y2 / t2)); // x좌표 구하기
  }
  else { // 2개의 비콘의 반경이 겹치지 않을 경우
    x2 = Length; // x좌표 구하기
    y2 = t2 + abs((Length - t1 - t2) / 2); // y좌표 구하기
  }
  //----------------------------------------------------------------------------------------------
  if (t3 + t4 > Length) { // 2개의 비콘의 반경이 겹칠경우
    x3 = Length - t3 + abs((t3 - t4) / 2); // x좌표 구하기
    y3 = Length - t4 * sin(acos(x3 / t4)); // y좌표 구하기
  }
  else { // 2개의 비콘의 반경이 겹치지 않을 경우
    x3 = t4 + abs((Length - t3 - t4) / 2); // x좌표 구하기
    y3 = Length; // y좌표 구하기
  }
  //-----------------------------------------------------------------
  if (t4 + t1 > Length) { // 2개의 비콘의 반경이 겹칠경우
    y4 = Length - t4 + abs((t4 - t1) / 2);
    x4 = sin(acos(y4 / t1)); // x좌표 구하기
  }
  else { // 2개의 비콘의 반경이 겹치지 않을 경우
    x4 = 0; // x좌표 구하기
    y4 = t1 + abs((Length - t1 - t4) / 2); // y좌표 구하기
  }
  //------------------------------------------------------------------

  int cnt1 = 0;
  int cnt2 = 0;

  if(x1 >= 1 and x1 <=Length){
    cnt1 = cnt1+1;
  }
  else{
    x1 = 0;
  }
  if(x2 >= 1 and x2 <=Length){
    cnt1 = cnt1+1;
  }
  else{
    x2 = 0;
  }
  if(x3 >= 1 and x3 <=Length){
    cnt1 = cnt1+1;
  }
  else{
    x3 = 0;
  }
  if(x4 >= 1 and x4 <=Length){
    cnt1 = cnt1+1;
  }
  else{
    x4 = 0;
  }
  if(y1 >= 1){
    cnt2 = cnt2+1;
  }
  else{
    y1 = 0.5;
  }
  if(y2 >= 1){
    cnt2 = cnt2+1;
  }
  else{
    y2 = 0;
  }
  if(y3 >= 1){
    cnt2 = cnt2+1;
  }
  else{
    y3 = 0;
  }
  if(y4 >= 1){
    cnt2 = cnt2+1;
  }
  else{
    y4 = 0;
  }

  x = (x1 + x2 + x3 + x4) / cnt1; // 나온 x좌표 4개 산술평균
  y = (y1 + y2 + y3 + y4) / cnt2; // 나온 y좌표 4개 산술평균


  //가장자리 보정
  if (t1<Length/5 and t2<Length/5 and t3>Length/2 and t4>Length/2){
    y = Length/10;
  }
  if (t2<Length/5 and t3<Length/5 and t4>Length/2 and t1>Length/2){
    x = Length - Length/10;
  }
  if (t3<Length/5 and t4<Length/5 and t1>Length/2 and t2>Length/2){
    y = Length - Length/10;
  }
  if (t4<Length/5 and t1<Length/5 and t2>Length/2 and t3>Length/2){
    x = Length/10;
  }

  //꼭지점 보정
  if (t1<Length/5 and t2>Length/2 and t3>Length/2 and t4>Length/2){
    x = Length/10;
    y = Length/10;  
  }
  if (t2<Length/5 and t1>Length/2 and t3>Length/2 and t4>Length/2){
    x = Length - Length/10;
    y = Length/10;
  }
  if (t3<Length/5 and t1>Length/2 and t2>Length/2 and t4>Length/2){
    x = Length - Length/10;
    y = Length - Length/10;  
  }
  if (t4<Length/5 and t1>Length/2 and t2>Length/2 and t3>Length/2){
    x = Length/10;
    y = Length - Length/10;
  }
  


  Serial.print("b[0] = "); Serial.println(b[0]);
  Serial.print("b[1] = "); Serial.println(b[1]);
  Serial.print("b1 = "); Serial.println(b1);
  Serial.print("b2 = "); Serial.println(b2);
  Serial.print("c0 = "); Serial.println(b0);
  Serial.print("c[0] = "); Serial.println(c[0]);
  Serial.print("c[1] = "); Serial.println(c[1]);
  Serial.print("c1 = "); Serial.println(c1);
  Serial.print("c2 = "); Serial.println(c2);
  Serial.print("c0 = "); Serial.println(c0);
  Serial.print("d[0] = "); Serial.println(c[0]);
  Serial.print("d[1] = "); Serial.println(c[1]);
  Serial.print("d1 = "); Serial.println(d1);
  Serial.print("d2 = "); Serial.println(d2);
  Serial.print("d0 = "); Serial.println(d0);
  Serial.print("e0 = "); Serial.println(e0);
  Serial.print("e[0] = "); Serial.println(e[0]);
  Serial.print("e[1] = "); Serial.println(e[1]);
  Serial.print("e1 = "); Serial.println(e1);
  Serial.print("e2 = "); Serial.println(e2);
  Serial.print("e0 = "); Serial.println(e0);
  Serial.println("");
  Serial.println("------------------------결과------------------------");
  Serial.println("");
  Serial.print("(0,0)지점 RSSI : b0 = "); Serial.println(b0);
  Serial.print("t1 = "); Serial.println(t1);
  Serial.println("");
  Serial.print("(Length,0)지점 RSSI : c0 = "); Serial.println(c0);
  Serial.print("t2 = "); Serial.println(t2);
  Serial.println("");
  Serial.print("(Length,Length)지점 RSSI : d0 = "); Serial.println(d0);
  Serial.print("t3 = "); Serial.println(t3);
  Serial.println("");
  Serial.print("(0,Length)지점 RSSI : e0 = "); Serial.println(e0);
  Serial.print("t4 = "); Serial.println(t4);
  Serial.println("");
  Serial.print("x1 = "); Serial.print(x1); Serial.print("  "); Serial.print("y1 = "); Serial.println(y1);
  Serial.print("x2 = "); Serial.print(x2); Serial.print("  "); Serial.print("y2 = "); Serial.println(y2);
  Serial.print("x3 = "); Serial.print(x3); Serial.print("  "); Serial.print("y3 = "); Serial.println(y3);
  Serial.print("x4 = "); Serial.print(x4); Serial.print("  "); Serial.print("y4 = "); Serial.println(y4);

  Serial.print("x = "); Serial.print(x); Serial.print("  "); Serial.print("y = "); Serial.println(y);
  
  lcd.setCursor(0, 0); // 해당 LCD 좌표로 커서 이동
  lcd.print("x = ");
  lcd.print(x);
  lcd.println("");
  lcd.setCursor(0, 1); // 해당 LCD 좌표로 커서 이동
  lcd.print("y = ");
  lcd.print(y);
  lcd.println("");
}
