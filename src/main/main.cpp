#include "Arduino.h"
#include "main.h"
#include <i2c_t3.h>

const uint8_t target_write = 0x4E;
const uint8_t target_read = 0x4F;
//NOTE: values valid when IOCON.BANK = 0 on MCP23018
const uint8_t IODIRA = 0x00;
const uint8_t IODIRB = 0x01;
const uint8_t GPPUA = 0x0C;
const uint8_t GPPUB = 0x0D;
const uint8_t GPIOA = 0x12;
const uint8_t GPIOB = 0x13;

const int ROW_PIN_START = 8;
const int ROW_PIN_END = 9;//10;
const int COL_PIN_START = 2;
const int COL_PIN_END = 3;//7;

const int ROW_LEN = 3;
const int COL_LEN = 12;
const int FN_LEVELS = 4;

//fn levels 0, 1, 2, 3 (3 is only for left half)
int fn_l = 0;
int fn_r = 0;

int_stack free_usb_keys;

key_elem_t key_map[COL_LEN*ROW_LEN*FN_LEVELS] =
{
  //level 0
  {HIGH, 0, KEY_Q, -1}, {HIGH, 0, KEY_W, -1}, {HIGH, 0, 0, -1}, {HIGH, 0, 0, -1}, {HIGH, 0, 0, -1}, {HIGH, 0, 0, -1},
    {HIGH, 0, KEY_A, -1}, {HIGH, 0, KEY_W, -1}, {HIGH, 0, 0, -1}, {HIGH, 0, 0, -1}, {HIGH, 0, 0, -1}, {HIGH, 0, 0, -1},
  {HIGH, 0, KEY_A, -1}, {HIGH, 0, KEY_W, -1}, {HIGH, 0, 0, -1}, {HIGH, 0, 0, -1}, {HIGH, 0, 0, -1}, {HIGH, 0, 0, -1},
    {HIGH, 0, KEY_A, -1}, {HIGH, 0, KEY_W, -1}, {HIGH, 0, 0, -1}, {HIGH, 0, 0, -1}, {HIGH, 0, 0, -1}, {HIGH, 0, 0, -1},
  //level 1
  {HIGH, 0, KEY_Q, -1}, {HIGH, 0, KEY_W, -1}, {HIGH, 0, 0, -1}, {HIGH, 0, 0, -1}, {HIGH, 0, 0, -1}, {HIGH, 0, 0, -1},
    {HIGH, 0, KEY_Q, -1}, {HIGH, 0, KEY_W, -1}, {HIGH, 0, 0, -1}, {HIGH, 0, 0, -1}, {HIGH, 0, 0, -1}, {HIGH, 0, 0, -1},
  {HIGH, 0, KEY_Q, -1}, {HIGH, 0, KEY_W, -1}, {HIGH, 0, 0, -1}, {HIGH, 0, 0, -1}, {HIGH, 0, 0, -1}, {HIGH, 0, 0, -1},
    {HIGH, 0, KEY_A, -1}, {HIGH, 0, KEY_W, -1}, {HIGH, 0, 0, -1}, {HIGH, 0, 0, -1}, {HIGH, 0, 0, -1}, {HIGH, 0, 0, -1},
  //level 2
  {HIGH, 0, KEY_Q, -1}, {HIGH, 0, KEY_W, -1}, {HIGH, 0, 0, -1}, {HIGH, 0, 0, -1}, {HIGH, 0, 0, -1}, {HIGH, 0, 0, -1},
    {HIGH, 0, KEY_Q, -1}, {HIGH, 0, KEY_W, -1}, {HIGH, 0, 0, -1}, {HIGH, 0, 0, -1}, {HIGH, 0, 0, -1}, {HIGH, 0, 0, -1},
  {HIGH, 0, KEY_Q, -1}, {HIGH, 0, KEY_W, -1}, {HIGH, 0, 0, -1}, {HIGH, 0, 0, -1}, {HIGH, 0, 0, -1}, {HIGH, 0, 0, -1},
    {HIGH, 0, KEY_A, -1}, {HIGH, 0, KEY_W, -1}, {HIGH, 0, 0, -1}, {HIGH, 0, 0, -1}, {HIGH, 0, 0, -1}, {HIGH, 0, 0, -1},
  //level 3
  {HIGH, 0, KEY_Q, -1}, {HIGH, 0, KEY_W, -1}, {HIGH, 0, 0, -1}, {HIGH, 0, 0, -1}, {HIGH, 0, 0, -1}, {HIGH, 0, 0, -1},
    {HIGH, 0, KEY_Q, -1}, {HIGH, 0, KEY_W, -1}, {HIGH, 0, 0, -1}, {HIGH, 0, 0, -1}, {HIGH, 0, 0, -1}, {HIGH, 0, 0, -1},
  {HIGH, 0, KEY_Q, -1}, {HIGH, 0, KEY_W, -1}, {HIGH, 0, 0, -1}, {HIGH, 0, 0, -1}, {HIGH, 0, 0, -1}, {HIGH, 0, 0, -1},
    {HIGH, 0, KEY_A, -1}, {HIGH, 0, KEY_W, -1}, {HIGH, 0, 0, -1}, {HIGH, 0, 0, -1}, {HIGH, 0, 0, -1}, {HIGH, 0, 0, -1},  
};


void init_main() {
  //start serial connection
  Serial.begin(9600);
  //pins 2, 3, 4, 5, 6, 7 correspond to columns for the half with the mcu.
  pinMode(2, INPUT_PULLUP);
  pinMode(3, INPUT_PULLUP);
  pinMode(4, INPUT_PULLUP);
  pinMode(5, INPUT_PULLUP);
  pinMode(6, INPUT_PULLUP);
  pinMode(7, INPUT_PULLUP);
  //pins 8, 9, 10 correspond to rows for the half with thew mcu.
  pinMode(8, OUTPUT_OPENDRAIN);
  pinMode(9, OUTPUT_OPENDRAIN);
  pinMode(10, OUTPUT_OPENDRAIN);
  
  // Setup for Master mode, pins 18/19, external pullups, 400kHz, 200ms default timeout
  Wire.begin(I2C_MASTER, 0x00, I2C_PINS_18_19, I2C_PULLUP_INT, 400000);
  Wire.setDefaultTimeout(200000); // 200ms

  //free_usb_keys.push(6);
  free_usb_keys.push(5);
  free_usb_keys.push(4);
  free_usb_keys.push(3);
  free_usb_keys.push(2);
  free_usb_keys.push(1);

  // configure the io expander
  delay(10);
  Wire.beginTransmission(target_write);
  //gpiob set as outputs
  Wire.write(IODIRB);
  Wire.write(0x00);
  Wire.endTransmission();
  delay(10);

  // Check if error occured
        if(Wire.getError())
            Serial.print("i2c conf1 FAIL\n");
        else
            Serial.print("i2c conf1 OK\n");
  
  //gpioa set to use internal pull-ups
  Wire.beginTransmission(target_read);
  Wire.write(GPPUA);
  Wire.write(0xFF);
  Wire.endTransmission();

  // Check if error occured
        if(Wire.getError())
            Serial.print("i2c conf2 FAIL\n");
        else
            Serial.print("i2c conf2 OK\n");
}

unsigned int key_addr(int x, int y, int l){
  int offs = l*(12*3);
  
  return offs+12*y+x;
}

void set_row_left(int row){
      //Serial.println(row_pin);
    digitalWrite(8, HIGH);
    digitalWrite(9, HIGH);
    digitalWrite(10, HIGH);
    digitalWrite(row + ROW_PIN_START, LOW);
}

void set_row_right(int row){
  uint8_t output_row = 0xFE;
  output_row << row;
  
  Wire.beginTransmission(target_write);
  Wire.write(GPIOA);
  Wire.write(output_row);
  Wire.endTransmission();  
}

void get_row_right(int *row_buff){
  Wire.beginTransmission(target_read);
  Wire.write(GPIOB);
  //Wire.requestFrom(target_read, row_buff);
  Wire.endTransmission();
}

void get_row_left(int *row_buff){
  int col;
  for (; col<COL_LEN; ++col){
    row_buff[col] = digitalRead(COL_PIN_START + col);
  }
}

key_elem_t *get_key(int row, int col, int fn_l, int *row_buff){
  unsigned int addr = key_addr(
                        col,
                        row,
                        fn_l);
  return &key_map[addr];
}


int set_key(key_elem_t *k, int val){
  int prev_val = k->val;
  int usb_key = -1;
  
  if (val == LOW && prev_val == HIGH){
    usb_key = free_usb_keys.pop();
    k->usb_key = usb_key;
  }
  else if (val == HIGH && prev_val == LOW){
    usb_key = k->usb_key;
    free_usb_keys.push(usb_key);   
  }
  
  k->val = val;
  return usb_key;
}

int modifier = 0;

void reset_usb(){
  modifier = 0;
}

void set_usb(key_elem_t *k, int usb_key){
  //usb hid keyboard only allows 6 keypresses at once
  
  if (usb_key >= 6 || usb_key < 1)
    return;
  
  modifier |= k->modifier;
  Keyboard.set_modifier(modifier);

  int key = 0;
  if (k->val == LOW)
  {
    key = k->normal_key;
  }
  
  switch(usb_key)
  {
    case 0:
      break;
    case 1:
      Keyboard.set_key2(key);
      break;
    case 2:
      Keyboard.set_key3(key);
      break;
    case 3:
      Keyboard.set_key4(key);
      break;
    case 4:
      Keyboard.set_key5(key);
      break;
    case 5:
      Keyboard.set_key6(key);
      break;
    default:
      ;
  }
}

void send_usb(){
  Keyboard.send_now();
}

void run_main() {
  int row_buff[COL_LEN];
  reset_usb();
  
  int row = 0;
  for (;row<ROW_LEN;++row){
    set_row_right(row);
    set_row_left(row);
    get_row_right(row_buff);
    get_row_left(row_buff + COL_LEN/2);
    
    int col = 0;
    for (;col<COL_LEN;++col){
      key_elem_t *k = get_key(row, col, fn_l, row_buff);
      int key_val = row_buff[col];
      if (key_val !=  k->val){
        int usb_key = set_key(k, key_val);
        set_usb(k, usb_key);
        send_usb();
      }
      else{
        //nothing has changed, do nothing.
        ;
      }
    }
  }
  
  delay(50);
}
