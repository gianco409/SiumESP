#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <Wire.h>
#include "RTClib.h"

#define DEFAULT_VIBRATION 1000

const char* ssid        = ".........";
const char* password    = ".........";
const char* mqtt_server = "xxx-xx-xxx-xxx-xxx.us-east-2.compute.amazonaws.com";

const int led_rojo  =  0;
const int led_azul  = 12;
const int led_verde = 16;
const int vibr_Pin  = 14;

bool flag1              = false;
char *topic             = "/soldexa/prensa/1";
String codigoMaquinaria = "1";
String horaInicio;
String horaFin;
int turno;

WiFiClient espClient;
PubSubClient client( espClient );
RTC_DS3231 rtc;

void setup_wifi() {
  digitalWrite( led_rojo, LOW );
  delay( 10 );
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print( "Connecting to " );
  Serial.println( ssid );
  WiFi.mode( WIFI_STA );
  WiFi.begin( ssid, password );

  while ( WiFi.status() != WL_CONNECTED ) {
    delay( 500 );
    Serial.print( "." );
  }

  randomSeed( micros() );

  Serial.println( "" );
  Serial.println( "WiFi connected" );
  Serial.println( "IP address: " );
  Serial.println( WiFi.localIP() );
  digitalWrite( led_rojo,HIGH );
}

void reconnect() {
  // Loop until we're reconnected
  digitalWrite( led_azul, LOW );
  while ( !client.connected() ) {
    Serial.print( "Attempting MQTT connection..." );
    // Create a random client ID
    String clientId = "ESP8266Client-";
    clientId += String( random(0xffff), HEX );
    // Attempt to connect
    if ( client.connect( clientId.c_str() ) ) {
      Serial.println( "connected" );
      digitalWrite( led_azul,HIGH );
    } else {
      Serial.print( "failed, rc=" );
      Serial.print( client.state() );
      Serial.println( " try again in 0.5 seconds" );
      enviarPaqueteEstado( -1 );
      // Wait 0.5 seconds before retrying
      delay( 500 );
    }
  }
}

bool enviarPaqueteCambio(){
  digitalWrite( led_verde, LOW );
  String aux = "CAMBIO." + codigoMaquinaria + "." +
                turno + "." + horaInicio + "." + horaFin;
  char paquete[200];
  aux.toCharArray( paquete, 200 );
  bool flag = client.publish( topic, paquete );
  
  if(flag){
    Serial.println( paquete );
    digitalWrite( led_verde, HIGH );
    delay( 1000 );
    return true;
  }else{
    Serial.println( "mensaje no enviado" );
    return false;
  }
}

bool enviarPaqueteEstado( int estado ){
  //digitalWrite( led_rojo, LOW );
  String aux = "ESTADO." + codigoMaquinaria + "." + estado;
  char paquete[200];
  aux.toCharArray( paquete, 200 );
  bool flag = client.publish( topic, paquete );
  
  if( flag ){
    Serial.println( paquete );
    //digitalWrite(led_rojo, HIGH);
    delay( 1000 );
  }else{
    Serial.println( "mensaje no enviado" );
  }
}

void callback( char* topic, byte* payload, unsigned int length ) {
  Serial.print( "Message arrived [" );
  Serial.print( topic );
  Serial.print( "] " );
  for ( int i = 0; i < length; i++ ) {
    Serial.print( (char) payload[i] );
  }
  Serial.println();

  // Switch on the LED if an 1 was received as first character
  if ( (char) payload[0] == '1' ) {
    digitalWrite( BUILTIN_LED, LOW );   // Turn the LED on (Note that LOW is the voltage level
    // but actually the LED is on; this is because
    // it is acive low on the ESP-01)
  } else {
    digitalWrite( BUILTIN_LED, HIGH );  // Turn the LED off by making the voltage HIGH
  }

}

long TP_init(){
  delay( 10 );
  long measurement = pulseIn ( vibr_Pin, HIGH );  //wait for the pin to get HIGH and returns measurement
  return measurement;
}

void setup() {
  pinMode( led_verde, OUTPUT );
  pinMode( led_azul, OUTPUT );
  setup_wifi();
  client.setServer( mqtt_server, 1883 );
  client.setCallback( callback );
  if ( !rtc.begin() ) {
    Serial.println( "No hay un módulo RTC" );
  }
  // Ponemos en hora, solo la primera vez, luego comentar y volver a cargar.
  // Ponemos en hora con los valores de la fecha y la hora en que el sketch ha sido compilado.
  // rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  pinMode( vibr_Pin, INPUT );
  Serial.begin( 115200 );
}

void loop() {
  if ( !client.connected() ) {
    reconnect();
  }
  digitalWrite(led_verde, LOW);
  client.loop();
  long sensor = TP_init();
  delay(50);
  enviarPaqueteEstado( sensor );
  if ( sensor >= DEFAULT_VIBRATION && !flag1 ){
    DateTime now = rtc.now();
    horaInicio = String(now.hour()) + ":" + String(now.minute()) + ":" + String(now.second());
    flag1 = true;
  }
  else if( sensor < DEFAULT_VIBRATION && flag1 ){
    DateTime now = rtc.now();
    horaFin = String(now.hour()) + ":" + String(now.minute()) + ":" + String(now.second());
    enviarPaqueteCambio();
    flag1 = false;
  }
  delay( 200 );
}
