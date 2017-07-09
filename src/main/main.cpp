#include "Arduino.h"
#include "main.h"
#include <i2c_t3.h>
#include "key_codes.h"

const uint8_t target_write = 0x4E;
const uint8_t target_read = 0x4F;
//NOTE: values valid when IOCON.BANK = 0 on MCP23018
const uint8_t IODIRA = 0x00;
const uint8_t IODIRB = 0x01;
const uint8_t GPPUA = 0x0C;
const uint8_t GPPUB = 0x0D;
const uint8_t GPIOA = 0x12;
const uint8_t GPIOB = 0x13;

const int ROW_LEN = 4;
const int COL_LEN = 12;
const int N_ROW_PINS = 4;
const int N_COL_PINS = 6;
const int COL_PINS[] = {0, 1, 2, 3, 4, 5};
const int ROW_PINS[] = {6, 7, 8, 9};
const int FN_LEVELS = 4;

//fn levels 0, 1, 2, 3 (3 is only for left half)
int fn_l = 0;
int fn_r = 0;

int_stack free_usb_keys;

const int ledPin = 13;

//dvorak
//1234567890[]
//',.pyfgcrl/=
//aoeuidhtns-\
//;qjkxbmwvz
//swedish
//1234567890+´
//qwertyuiopå¨
//asdfghjklöä'
//<zxcvbnm,.-
//us
//...
key_val_t key_map[COL_LEN*ROW_LEN*FN_LEVELS] =
{
  //level 0
  {0, KEY_SW_QUOTE}, {0, KEY_SW_COMMA}, {0, KEY_SW_PUNCT}, {0, KEY_P}, {0, KEY_Y},
    {0, KEY_F}, {0, KEY_G}, {0, KEY_C}, {0, KEY_R}, {0, KEY_L}, {MODIFIERKEY_SHIFT, KEY_7}, {MODIFIERKEY_SHIFT, KEY_0},
  {0, KEY_A}, {0, KEY_O}, {0, KEY_E}, {0, KEY_U}, {0, KEY_I}, {0, KEY_D},
    {0, KEY_H}, {0, KEY_T}, {0, KEY_N}, {0, KEY_S}, {0, KEY_SW_KEY_HYPH}, {0, KEY_SW_PLUS},
  {MODIFIERKEY_SHIFT, KEY_COMMA}, {0, KEY_Q}, {0, KEY_J}, {0, KEY_K}, {0, KEY_X},
    {0, KEY_B}, {0, KEY_M}, {0, KEY_W}, {0, KEY_V}, {0, KEY_Z},
  //level 1
  {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, 
    {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, 
  {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, 
    {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, 
  {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, 
    {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, 
  //level 2
  {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, 
    {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, 
  {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, 
    {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, 
  {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, 
    {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0},
    //level 3
  {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, 
    {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, 
  {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, 
    {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, 
  {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, 
    {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0},
   //level 4
  {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, 
    {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, 
  {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, 
    {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, 
  {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, 
    {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, 
};

bool key_status[COL_LEN*ROW_LEN] = {0};


void init_main() {
  Serial.println("Begin Init");
 
  //Led used for debugging
  pinMode(ledPin, OUTPUT);
  
  //start serial connection
  Serial.begin(38400);
  //pins 0, 1, 2, 3, 4, 5 correspond to columns for the half with the mcu.
  int r=0;
  for (r;r<N_ROW_PINS;++r){
    pinMode(ROW_PINS[r], OUTPUT);
    digitalWrite(ROW_PINS[r], LOW);
  }

  int c = 0;
  for (;c<N_COL_PINS;++c){
    pinMode(COL_PINS[c], INPUT_PULLDOWN);
    //digitalWrite(COL_PINS[c], LOW);
  }

  // Setup for Master mode, pins 18/19, external pullups, 400kHz, 200ms default timeout
  Wire.begin(I2C_MASTER, 0x00, I2C_PINS_18_19, I2C_PULLUP_INT, 400000);
  Wire.setDefaultTimeout(200000); // 200ms

  free_usb_keys.push(5);
  free_usb_keys.push(4);
  free_usb_keys.push(3);
  free_usb_keys.push(2);
  free_usb_keys.push(1);
  free_usb_keys.push(0);

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
  
  Serial.println("Finish Init");
}

unsigned int key_addr(int x, int y, int l){
  int offs = l*(12*3);
  
  return offs+12*y+x;
}

void write_matrix_local(int row){
      //Serial.println(row_pin);
    int i=0;
    int val = HIGH;
    for (;i<ROW_LEN;++i)
    {
      if (i == row)
        val = HIGH;
      else
        val = LOW;
      digitalWrite(ROW_PINS[i], val);
    }

}

void set_row_remote(int row){
  uint8_t output_row = 0xFE;
  output_row << row;
  
  Wire.beginTransmission(target_write);
  Wire.write(GPIOA);
  Wire.write(output_row);
  Wire.endTransmission();  
}

void get_row_remote(int *row_buff){
  Wire.beginTransmission(target_read);
  Wire.write(GPIOB);
  //Wire.requestFrom(target_read, row_buff);
  Wire.endTransmission();
}

void read_matrix_local(int *row_buff){
  int col;
  for (; col<COL_LEN; ++col){
    int col_pin = COL_PINS[col];
    row_buff[col] = digitalRead(col_pin);
  }
}

key_elem_t *get_key(int row, int col, int fn_l, int *row_buff){
  unsigned int addr = key_addr(
                        col,
                        row,
                        fn_l);
  return &key_map[addr];
}


int get_free_usb_key(key_elem_t *k, int val){
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
  if (k->val == HIGH)
  {
    key = k->normal_key;
  }

  keyboard_keys[usb_key] = key;
}

void send_usb(){
  Keyboard.send_now();
}

void run_main() {
  int row_buff[COL_LEN];
  reset_usb();
  
  int row = 0;
  for (;row<ROW_LEN;++row){
    //set_row_remote(row);
    write_matrix_local(row);
    //get_row_remote(row_buff);
    read_matrix_local(row_buff);

    int col = 0;
    for (;col<COL_LEN;++col){
      key_elem_t *k = get_key(row, col, fn_l, row_buff);
      int key_val = row_buff[col];
      if (key_val !=  k->val){
        Serial.print(col);
        Serial.print("  ");
        Serial.print(row);
        Serial.println("");
        int usb_key = set_key(k, key_val);
        set_usb(k, usb_key);
        send_usb();
      }
      else{
        //nothing has changed, do nothing.
      }
    }
  }
  
  delay(10);  
}
