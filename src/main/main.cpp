#include "Arduino.h"
#include "main.h"
#include <i2c_t3.h>
#include "key_codes.h"

const int UNASSIGNED_DOWN = -2;
const int UNASSIGNED_UP = -1;
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

const int N_ROWS = 4;
const int N_COLS = 12;
const int N_ROW_PINS = 4;
const int N_COL_PINS = 6;
const int COL_PINS[] = {0, 1, 2, 3, 4, 5};
const int ROW_PINS[] = {6, 7, 8, 9};
const int LED_PIN = 13;

const int N_FN_LAYERS = 4;
const int MAX_N_USB_KEYS = 6;

//fn levels 0, 1, 2, 3
int fn_l = 0;
int fn_r = 0;
int fn_layer = 0;
int n_pressed_keys = 0;

int_stack free_usb_keys;

//Counters for all modifiers showing how many keys affect them 
int modifier_counters[N_MODIFIERS] = {0};

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
key_data_t key_map[N_COLS*N_ROWS*N_FN_LAYERS] =
{
  //level 0
  {MODIFIER_NONE, KEY_SW_QUOTE}, {MODIFIER_NONE, KEY_SW_COMMA}, {MODIFIER_NONE, KEY_SW_PUNCT}, {MODIFIER_NONE, KEY_P}, {MODIFIER_NONE, KEY_Y}, {MODIFIER_NONE, KEY_F},
    {MODIFIER_NONE, KEY_G}, {MODIFIER_NONE, KEY_C}, {MODIFIER_NONE, KEY_R}, {MODIFIER_NONE, KEY_L}, {MODIFIER_LSHIFT, KEY_7}, {MODIFIER_LSHIFT, KEY_0},
  {MODIFIER_NONE, KEY_A}, {MODIFIER_NONE, KEY_O}, {MODIFIER_NONE, KEY_E}, {MODIFIER_NONE, KEY_U}, {MODIFIER_NONE, KEY_I}, {MODIFIER_NONE, KEY_D},
    {MODIFIER_NONE, KEY_H}, {MODIFIER_NONE, KEY_T}, {MODIFIER_NONE, KEY_N}, {MODIFIER_NONE, KEY_S}, {MODIFIER_NONE, KEY_SW_HYPH}, {MODIFIER_NONE, KEY_SW_PLUS},
  {MODIFIER_LSHIFT, KEY_COMMA}, {MODIFIER_NONE, KEY_Q}, {MODIFIER_NONE, KEY_J}, {MODIFIER_NONE, KEY_K}, {MODIFIER_NONE, KEY_X}, {MODIFIER_NONE, KEY_BACKSPACE},
    {MODIFIER_NONE, KEY_B}, {MODIFIER_NONE, KEY_M}, {MODIFIER_NONE, KEY_W}, {MODIFIER_NONE, KEY_V}, {MODIFIER_NONE, KEY_Z}, {MODIFIER_NONE, KEY_ENTER},
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

int key_status[N_COLS*N_ROWS];
unsigned int modifier;

unsigned int key_address(int col, int row, int layer){
  return layer*N_COLS*N_ROWS + row*N_COLS + col;
}

void set_int_array(int *arr, size_t n_elems, int val){
  size_t i=0;
  for (;i<n_elems;++i){
    arr[i] = val;
  }  
}

void reset_key_statuses(){
  set_int_array(key_status, N_COLS*N_ROWS, UNASSIGNED_UP);
}

void print_int_array(int *arr, size_t n_elems){
  size_t i=0;
  for (;i<n_elems;++i){
    Serial.print(arr[i]);
    Serial.print(", ");
  }
  Serial.println("");
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

  reset_key_statuses();
  set_int_array(modifier_counters, N_MODIFIERS, 0);
  modifier=0;
  Serial.println("Finish Init");
}


void write_matrix_local(int row){
    int i=0;
    int val = HIGH;
    for (;i<N_ROW_PINS;++i)
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


int get_free_key_nr(){
  if (free_usb_keys.empty())
    return -1;
   
  return free_usb_keys.pop();
}

void return_key_nr(int key_nr){
  free_usb_keys.push(key_nr);
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

bool should_update(int key_state, int old_status){
  if ( 
      //Now: pressed, before: unassigned 
      (key_state == HIGH && old_status == UNASSIGNED_UP)
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

  return false;
}

bool set_usb_key(int usb_key, int value){
  if (usb_key >= 0 and usb_key <= MAX_N_USB_KEYS){
    keyboard_keys[usb_key] = value;
    return true;
  }
  return false;
}

unsigned int update_modifiers(int key_state, int modifier){
  unsigned int res = 0;
   
  if (modifier != MODIFIER_NONE){
    int counter = modifier_counters[modifier];
    if (key_state == HIGH){
      
      if (counter == 0){
        bitSet(res, modifier);
      }
        
      modifier_counters[modifier] = counter + 1;
    }
    else{
      if (counter == 1){
        bitClear(res,modifier);
      }
      modifier_counters[modifier] = counter - 1;
    }
  }

  Serial.println("MODIFIERS after");

  print_int_array(modifier_counters, N_MODIFIERS);
  Serial.println(res, BIN);
  return res;
}

void activate_hooks(){
  ;
}

void run_main() {
  
  int row_buff[N_COLS] = {0};
  set_int_array(row_buff, N_COLS, LOW);
  
  int row = 0;
  for (;row<N_ROWS;++row){
    //set_row_remote(row);
    write_matrix_local(row);
    //get_row_remote(row_buff);
    read_matrix_local(row_buff);

    //print_int_array(row_buff, N_COLS);
    
    int col = 0;
    for (;col<N_COLS;++col){
      int key_state = row_buff[col]; //HIGH, LOW
      int key_nr = get_key_status(col, row); //-1, 0, 1, 2, 3, 4, 5
      int next_key_nr = UNASSIGNED_UP;
      key_data_t key_data = get_key_data(col, row);
      
      if (should_update(key_state, key_nr)){
        Serial.print(col);
        Serial.print("  ");
        Serial.print(row);
        Serial.println(" B");
        int usb_key_value=0;
        bool should_update_usb = false;
        
        if (key_state == HIGH){
          Serial.println("KEY DOWN");
          key_nr = get_free_key_nr();
          next_key_nr = key_nr;
          usb_key_value = key_data.hid_code;
          ++n_pressed_keys;
        }
        else{
          Serial.println("KEY UP");
          return_key_nr(key_nr);     
          next_key_nr = UNASSIGNED_UP;
          usb_key_value = HID_RELEASED;
          --n_pressed_keys;
        }

        should_update_usb = set_usb_key(key_nr, usb_key_value);        
        Serial.print("key_nr ");
        Serial.print(key_nr);
        Serial.print(" usb_key ");
        Serial.print(usb_key_value);
        Serial.println(" 3");

        unsigned int new_modifier = update_modifiers(key_state, key_data.modifier);
        
        Serial.print("new_modifier ");
        Serial.print(new_modifier, BIN);
        Serial.print("modifier");
        Serial.println(modifier, BIN);
        if (new_modifier != modifier){
          Serial.println("Update modifier");
          should_update_usb = true;
          modifier=new_modifier;
          Keyboard.set_modifier(new_modifier);
        }
          
        activate_hooks();
        
        if (should_update_usb){
          send_usb(); 
        }

        Serial.print("next status");
        Serial.println(next_key_nr);
        set_status(col, row, next_key_nr);
      }
    }
  }  
}
