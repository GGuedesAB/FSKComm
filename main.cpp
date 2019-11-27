#include <Arduino.h>
#define N 100
#define M 800
bool recieved_a_bit = false;
bool can_send_a_bit = true;
bool first_A_interruption = true;
bool first_B_interruption = true;
int send_counter = 0;
int current_bit = 0;
volatile uint8_t send_bit = LOW;
volatile uint8_t recieve_bit = LOW;
uint8_t my_bits_to_send [M];

ISR(TIMER1_COMPA_vect) {
  if (first_A_interruption) {
    first_A_interruption = false;
    TCNT1 = 0;
  }
  else {
    // Sends data
    if ( current_bit > 0) {
      //digitalWrite(7, my_bits_to_send[current_bit]);
      Serial.print(my_bits_to_send[current_bit]);
      current_bit--;
    }
    else {
      current_bit = 0;
      disable_timer1();
    }
    // Clears the counter
    TCNT1 = 0;
  }
}

char recieve_char () {
  return 0;
}

bool send_char (char to_send) {
  send_counter++;
  current_bit++;
  uint8_t result = (uint8_t)to_send & (1<<(send_counter));
  if (send_counter < 7 && can_send_a_bit) {
    SREG &= ~0x80;
    if (result == 0) {
      send_bit = LOW;
    }
    else {
      send_bit = HIGH;
    }
    SREG |= 0x80;
    my_bits_to_send[current_bit] = send_bit;
    return false;
  }
  else if (send_counter == 7 && can_send_a_bit) {
    SREG &= ~0x80;
    if (result == 0) {
      send_bit = LOW;
    }
    else {
      send_bit = HIGH;
    }
    send_counter = -1;
    SREG |= 0x80;
    my_bits_to_send[current_bit] = send_bit;
    return true;
  }
  else {
    send_counter++;
    return false;
  }
}

void send_logic (char* serial_data, int data_size) {
  int i = 0;
  while (i < data_size) {
    if (send_char(serial_data[i]))
      i++;
  }
  Serial.println("Data sent.");
}

char *recieve_logic(char *serial_data, int data_size) {
  return NULL;
}

void enable_timer1 () {
  // Every 4ms we send one bit, this is done by OCR1A
  // 2ms after we check that bit we read it in the other port
  Serial.println("Enabling timers.");
  SREG &= ~0x80;
  TCNT1 = 0;
  TCCR1A ^= TCCR1A;
  TCCR1B ^= TCCR1B;
  TCCR1B |= (1<<WGM12);
  TCCR1B |= (1<<CS10);
  //TCCR1B |= (1<<CS12);
  TIFR1 ^= TIFR1;
  TIMSK1 |= (1<<OCIE1A);
  TCCR1C ^= TCCR1C;
  TCNT1 = 0;
  OCR1A = 64000;
  SREG |= 0x80;
}

void disable_timer1 () {
  Serial.println("Disabling timers.");
  TCNT1 = 0;
  TCCR1A ^= TCCR1A;
  TCCR1B ^= TCCR1B;
  TIMSK1 &= ~(1<<OCIE1A);
  TCNT1 = 0;
  OCR1A = 0;
  OCR1B = 0;
}

int get_string_size (char* my_string) {
  int i = 0;
  for (i=0; i<N; i++){
    if (my_string[i] == '\n'){
      return i - 1;
    }
  }
  exit (1);
}

void setup() {
  Serial.begin(115200);
  pinMode(12, INPUT);
  pinMode(7, OUTPUT);
}

void loop() {
  while (!Serial.available()){
    delay(10);
  }
  String to_send = Serial.readString();
  Serial.flush();
  Serial.println(to_send);
  char buff [N];
  to_send.toCharArray(buff, N);
  int size = get_string_size (buff);
  send_logic(buff, size);
  enable_timer1();
}