#include "Arduino.h"
#include "main.h"

const int ROW_PIN_START = 8;
const int ROW_PIN_END = 9;//10;
const int COL_PIN_START = 2;
const int COL_PIN_END = 3;//7;

//fn levels 0, 1, 2, 3 (3 is only for left half)
int fn_l = 0;
int fn_r = 0;

int_stack free_usb_keys;

key_elem_t key_map[12*3*4] =
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
  //pins 2, 3, 4, 5, 6, 7 correspond to columns for the holf with the mcu.
  pinMode(2, INPUT_PULLUP);
  pinMode(3, INPUT_PULLUP);
  pinMode(4, INPUT_PULLUP);
  pinMode(5, INPUT_PULLUP);
  pinMode(6, INPUT_PULLUP);
  pinMode(7, INPUT_PULLUP);
  //pins 8, 9, 10 correspond to rows for the half with the mcu.
  pinMode(8, OUTPUT_OPENDRAIN);
  pinMode(9, OUTPUT_OPENDRAIN);
  pinMode(10, OUTPUT_OPENDRAIN);

  //free_usb_keys.push(6);
  free_usb_keys.push(5);
  free_usb_keys.push(4);
  free_usb_keys.push(3);
  free_usb_keys.push(2);
  free_usb_keys.push(1);
}

unsigned int key_addr(int x, int y, int l){
  int offs = l*(12*3);
  
  return offs+12*y+x;
}

key_elem_t *get_key_left(int row_pin, int col_pin, int fn_l){
  unsigned int addr = key_addr(
                        col_pin - COL_PIN_START,
                        row_pin - ROW_PIN_START,
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
  reset_usb();
  
  int row_pin = ROW_PIN_START;
  for (;row_pin<=ROW_PIN_END;++row_pin){
    //Serial.println(row_pin);
    digitalWrite(8, HIGH);
    digitalWrite(9, HIGH);
    digitalWrite(10, HIGH);
    digitalWrite(row_pin, LOW);
    //wait for the current row to sink to low
    delayMicroseconds(50);
    
    int col_pin = COL_PIN_START;
    for (;col_pin<=COL_PIN_END;++col_pin){
      //Serial.println(col_pin);
      int key_val = digitalRead(col_pin);
      key_elem_t *k = get_key_left(row_pin, col_pin, fn_l);

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
