#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <Wire.h>
#include "RTClib.h"

#define DEFAULT_VIBRATION 500

const char* ssid        = ".........";
const char* password    = ".........";
const char* mqtt_server = "xxx-xx-xxx-xxx-xxx.us-east-2.compute.amazonaws.com";

const int led_rojo  = 16; // indica los cambios de estado
const int led_azul  =  0; // indica que se pudo conectar al broker
const int led_verde = 12; // indica que se pudo conectar a la red WIFI
const int vibr_Pin  = 14;

bool flag1              = false;
char *topic             = "/soldexa/prensa/1";
String codigoMaquinaria = "1";
String horaInicio;
String horaFin;
int turno;
int estado;

WiFiClient espClient;
PubSubClient client( espClient );
RTC_DS3231 rtc;

void setup_wifi() {
  delay( 10 );
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print( "Connecting to " );
  Serial.println( ssid );

  WiFi.begin( ssid, password );

  while ( WiFi.status() != WL_CONNECTED ) {
    delay( 500 );
    Serial.print( "." );
  }
  digitalWrite( led_verde, HIGH );
  Serial.println( "" );
  Serial.println( "WiFi connected" );
  Serial.println( "IP address: " );
  Serial.println( WiFi.localIP() );
}

void reconnect() {
  // Loop until we're reconnected
  digitalWrite( led_azul, LOW );
  while ( !client.connected() ) {
    Serial.print( "Attempting MQTT connection..." );
    // Create a random client ID
    String clientId = "ESP8266Client-";
    clientId += String( random( 0xffff ), HEX );
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

int getTurno(int h, int m ){
  //turno 1: 07:15 - 17:00
  //turno 3: 21:15 - 07:00
  if( ( h == 7 && m >= 15 ) || 
      ( h > 7 && h < 17 )  ||
      ( h == 17 && m == 0 ) ) {
    return 1;
  }
  
  if( ( h == 21 && m>=15 ) ||
      ( h > 21 && h <= 24 ) ||
      ( h >= 0 && h < 7 ) ||
      ( h == 7 && m == 0 ) ) {
    return 3;
  }
  
  return 2;
}

bool enviarPaqueteCambio( int h, int m ){
  digitalWrite( led_rojo, LOW );
  turno = getTurno(h,m);
  String aux = "CAMBIO." + codigoMaquinaria + "." +
                turno + "." + horaInicio + "." + horaFin;
  char paquete[200];
  aux.toCharArray( paquete, 200 );
  bool flag = client.publish( topic, paquete );
  
  if( flag ) {
    Serial.println( paquete );
    digitalWrite( led_rojo, HIGH );
    //delay( 1000 );
    return true;
  } else {
    Serial.println( "mensaje no enviado" );
    return false;
  }
}

bool enviarPaqueteEstado( int estado ){
  String aux = "ESTADO." + codigoMaquinaria + "." + estado;
  char paquete[200];
  aux.toCharArray( paquete, 200 );
  bool flag = client.publish( topic, paquete );
  
  if( flag ) {
    Serial.println( paquete );
  }else{
    Serial.println( "mensaje no enviado" );
  }
}

void callback( char* topic, byte* payload, unsigned int length ) {
  Serial.print( "Message arrived [" );
  Serial.print( topic );
  Serial.print( "] " );
  
  for ( int i = 0; i < length; i ++ ) {
    Serial.print( (char) payload[i] );
  }
  Serial.println();

  if ( (char) payload[0] == '1' ) {
    digitalWrite( BUILTIN_LED, LOW );
  } else {
    digitalWrite( BUILTIN_LED, HIGH );
  }
}

long TP_init(){
  delay( 10 );
  long measurement = pulseIn ( vibr_Pin, HIGH );
  return measurement;
}

void setup() {
  Serial.begin( 115200 );
  pinMode( led_verde, OUTPUT );
  pinMode( led_azul, OUTPUT );
  pinMode( led_rojo, OUTPUT );
  pinMode( vibr_Pin, INPUT );
  
  setup_wifi();
  
  client.setServer( mqtt_server, 1883 );
  client.setCallback( callback );
  if ( !rtc.begin() ) {
    Serial.println( "No hay un módulo RTC" );
  }
  // Ponemos en hora, solo la primera vez, luego comentar y volver a cargar.
  // Ponemos en hora con los valores de la fecha y la hora en que el sketch ha sido compilado.
  // rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
}

void loop() {
  if ( !client.connected() ) {
    reconnect();
  }
  
  client.loop();
  long val_sensor = TP_init();
  delay(50);

  int estado = val_sensor >= DEFAULT_VIBRATION ? 1 : 0;
  
  enviarPaqueteEstado( estado );
  
  if ( estado == 1 && !flag1 ) {
    DateTime now = rtc.now();
    horaInicio = String( now.hour() ) + ":" + 
                 String( now.minute() ) + ":" + 
                 String( now.second() );
    flag1 = true;
  } else if ( estado == 0 && flag1 ) {
    DateTime now = rtc.now();
    int h = now.hour();
    int m = now.minute();
    int s = now.second();
    horaFin = String( h ) + ":" + 
              String( m ) + ":" + 
              String( s );
    enviarPaqueteCambio( h, m );
    flag1 = false;
  }
  delay( 200 );
}
