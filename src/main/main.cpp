#include "Arduino.h"
#include "main.h"
#include <i2c_t3.h>
#include "key_codes.h"

const int UNASSIGNED_USB_KEY = -1;
const int HID_RELEASED = 0;

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
const int LED_PIN = 13;

const int fn_layerS = 4;
const int MAX_N_USB_KEYS = 6;

//fn levels 0, 1, 2, 3
int fn_l = 0;
int fn_r = 0;
int fn_layer = 0;
int n_pressed_keys = 0;

int_stack free_usb_keys;

int modifier = 0;

/*
  dvorak
  1234567890[]
  ',.pyfgcrl/=
  aoeuidhtns-\
  ;qjkxbmwvz
  swedish
  1234567890+´
  qwertyuiopå¨
  asdfghjklöä'
  <zxcvbnm,.-
  us
  ...
*/
key_data_t key_map[COL_LEN*ROW_LEN*fn_layerS] =
{
  //level 0
  {0, KEY_SW_QUOTE}, {0, KEY_SW_COMMA}, {0, KEY_SW_PUNCT}, {0, KEY_P}, {0, KEY_Y}, {0, KEY_F},
    {0, KEY_G}, {0, KEY_C}, {0, KEY_R}, {0, KEY_L}, {MODIFIERKEY_SHIFT, KEY_7}, {MODIFIERKEY_SHIFT, KEY_0},
  {0, KEY_A}, {0, KEY_O}, {0, KEY_E}, {0, KEY_U}, {0, KEY_I}, {0, KEY_D},
    {0, KEY_H}, {0, KEY_T}, {0, KEY_N}, {0, KEY_S}, {0, KEY_SW_HYPH}, {0, KEY_SW_PLUS},
  {MODIFIERKEY_SHIFT, KEY_COMMA}, {0, KEY_Q}, {0, KEY_J}, {0, KEY_K}, {0, KEY_X}, {0, 0},
    {0, KEY_B}, {0, KEY_M}, {0, KEY_W}, {0, KEY_V}, {0, KEY_Z}, {0, 0},
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
    {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}
};

int key_status[COL_LEN*ROW_LEN];

unsigned int key_address(int col, int row, int layer){
  return layer*COL_LEN*ROW_LEN + row*ROW_LEN + col;
}

void set_int_array(int *arr, size_t n_elems, int val){
  size_t i=0;
  for (;i<n_elems;++i){
    arr[i] = val;
  }  
}

void init_main() {
  Serial.println("Begin Init");
 
  //Led used for debugging
  pinMode(LED_PIN, OUTPUT);
  
  //start serial connection
  Serial.begin(38400);
  //pins 0, 1, 2, 3, 4, 5 correspond to columns for the half with the mcu.
  int r=0;
  for (;r<N_ROW_PINS;++r){
    pinMode(ROW_PINS[r], OUTPUT);
    digitalWrite(ROW_PINS[r], LOW);
  }

  int c = 0;
  for (;c<N_COL_PINS;++c){
    pinMode(COL_PINS[c], INPUT_PULLDOWN);
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

  set_int_array(key_status, COL_LEN*ROW_LEN, 0);
  
  Serial.println("Finish Init");
}


void write_matrix_local(int row){
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
  output_row <<= row;
  
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
  int col=0;
  int col_pin=0;
  for (; col<N_COL_PINS; ++col){
    col_pin = COL_PINS[col];
    row_buff[col] = digitalRead(col_pin);
  }
}


int get_free_usb_key(){
  if (free_usb_keys.empty())
    return -1;
   
  return free_usb_keys.pop();
}

void send_usb(){
  Keyboard.send_now();
}

int get_key_status(int col, int row){
  return key_status[key_address(col, row, 0)];
}

void set_status(int col, int row, int usb_key){
  key_status[key_address(col, row, 0)] = usb_key;
}

key_data_t get_key_data(int col, int row){
  return key_map[key_address(col,row,fn_layer)];
}

bool should_update(int key_state, int old_status, int new_modifier){
  //Cannot press any more keys
  if (key_state == HIGH && n_pressed_keys >= 6){
    return false;
  }


  if ( 
      //Now: pressed, before: unassigned 
      (key_state == HIGH && old_status == UNASSIGNED_USB_KEY)
      //Now: up, before: pressed and assigned
      ||(key_state == LOW && old_status >= 0 )
     ){
    Serial.print("key_state ");
    Serial.print(key_state);
    Serial.print(" old status ");
    Serial.print(old_status);
    Serial.println(" 2");
    return true;
  }

  if ((modifier | new_modifier) != modifier){
    //Serial.print("Modifier");
    //Serial.print(modifier);
    //return true;    
  }

  return false;
}

void set_usb_key(int usb_key, int hid_code, int new_modifier){
  if (usb_key >= 0 and usb_key <= MAX_N_USB_KEYS){
    keyboard_keys[usb_key] = hid_code;
    if (hid_code == HID_RELEASED)
      free_usb_keys.push(usb_key);
  }

  modifier |= new_modifier;
  Keyboard.set_modifier(modifier);
}

void activate_hooks(){
  ;
}

void run_main() {
  int row_buff[COL_LEN] = {0};
  set_int_array(row_buff, COL_LEN, LOW);
  
  int row = 0;
  for (;row<ROW_LEN;++row){
    //set_row_remote(row);
    write_matrix_local(row);
    //get_row_remote(row_buff);
    read_matrix_local(row_buff);

    
    int col = 0;
    for (;col<COL_LEN/2;++col){
      int hid_code=0;
      int key_state = row_buff[col]; //HIGH, LOW
      int old_usb_key = get_key_status(col, row); //-1, 0, 1, 2, 3, 4, 5
      key_data_t key_data = get_key_data(col, row);
      int new_modifier = key_data.modifier;
      
      if (should_update(key_state, old_usb_key, new_modifier)){
        Serial.print(col);
        Serial.print("  ");
        Serial.print(row);
        Serial.println(" A");
        int usb_key;
        if (key_state == HIGH){
          Serial.println("KEY DOWN");
          usb_key = get_free_usb_key();
          hid_code = key_data.hid_code;
          set_usb_key(usb_key, hid_code, new_modifier);
          ++n_pressed_keys;
        }
        else{
          Serial.println("KEY UP");
          set_usb_key(old_usb_key, HID_RELEASED, new_modifier);
          usb_key = -1;
          --n_pressed_keys;
        }
        Serial.print("usb_key ");
        Serial.print(usb_key);
        Serial.print(" hid_code ");
        Serial.print(hid_code);
        Serial.println(" 3");
        set_status(col, row, usb_key);
        activate_hooks();
        send_usb();
      }
      else{
        //nothing has changed, do nothing.
      }
    }
  }
  
  delay(10);  
}
