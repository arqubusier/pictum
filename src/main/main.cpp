#include "Arduino.h"
#include "main.h"
#include <i2c_t3.h>
#include "key_codes.h"

uint8_t g_modifiers = 0x00;

typedef struct stack_t{
  int head;
  int arr[MAX_LEN];
} stack_t;

void stack_init(stack_t *s){
  s->head = -1;
}


bool stack_is_empty(stack_t *s){
  return s->head == -1;
}
  
void stack_push(stack_t *s, int val){
  if (s->head < MAX_LEN){
    ++(s->head);
    s->arr[s->head] = val;
  }
}

int stack_pop(stack_t *s){
  if (stack_is_empty(s))
    return -1;
  
  int val = s->arr[s->head];
  --(s->head);
  return val;
}

int stack_size(stack_t *s){
  return s->head;
}

int stack_peek(stack_t *s){
  if (stack_is_empty(s)){
    return -1;
  }

  return s->arr[0];
}


stack_t free_keys;

//#define DEBUG

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
const int N_OUT_PINS = 4;
const int N_IN_PINS = 6;
const int REMOTE_COL_OFFS = N_COLS/2;
const int N_OUT_REMOTES = 6;
const int N_IN_REMOTES = 4;
const int COL_PINS[] = {0, 1, 2, 3, 4, 5};
const int ROW_PINS[] = {6, 7, 8, 9};
const int LED_PIN = 13;

const int N_FN_LAYERS = 4;
const int DEFAULT_FN_LAYER = 0;
const int MAX_N_USB_KEYS = 6;

//fn levels 0, 1, 2, 3
int fn_l = 0;
int fn_r = 0;
int fn_layer = 0;
int n_pressed_keys = 0;

int active_layers[N_FN_LAYERS] = {0};

//Counters for all modifiers showing how many keys affect them 
int modifier_counters[N_MODIFIERS] = {0};


#define TARGET 0x20 // target Slave0 address
#define IODIRA 0x00
#define IODIRB 0x01
#define GPIOA 0x12
#define GPIOB 0x13
#define GPPUA 0x0C
#define GPPUB 0x0D

int last_err = 0;

void i2c_init(){
    // Setup for Master mode, pins 29/30, external pullups, 400kHz, 200ms default timeout
    Wire1.begin(I2C_MASTER, 0x00, I2C_PINS_29_30, I2C_PULLUP_EXT, 100000);
    Wire1.setDefaultTimeout(200000); // 200ms
}

void mcp23018_write_reg(const uint8_t target, const uint8_t addr, const uint8_t d_in){
    Wire1.beginTransmission(target);   // Slave address
    Wire1.write(addr);
    Wire1.write(d_in);
    Wire1.endTransmission();
}

uint8_t mcp23018_read_reg(const uint8_t target, const uint8_t addr){
    Wire1.beginTransmission(target);
    Wire1.write(addr);
    Wire1.endTransmission();
    Wire1.requestFrom(target, (size_t)1);
    uint8_t dout = 0x00;

    last_err = Wire1.getError();
    //Check if error occured
    if(last_err){
        return 0;
    }
    else
    {
        while (Wire1.available())
            dout = Wire1.readByte();
    }

    return dout;
}

int mcp23018_read_status(){
  return last_err;
}

void mcp23018_init(){
  mcp23018_write_reg(TARGET, IODIRB, 0xFF);
  mcp23018_write_reg(TARGET, IODIRA, 0x00);
  mcp23018_write_reg(TARGET, GPPUB, 0xFF);
  mcp23018_write_reg(TARGET, GPPUA, 0x00);
}

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
  {MODIFIER_NONE, KEY_SW_QUOTE}, {MODIFIER_NONE, KEY_SW_COMMA}, {MODIFIER_NONE, KEY_SW_PUNCT}, {MODIFIER_NONE, KEY_SW_P}, {MODIFIER_NONE, KEY_SW_Y}, {MODIFIER_LCTRL, 0},
    {MODIFIER_NONE, KEY_SW_F}, {MODIFIER_NONE, KEY_SW_G}, {MODIFIER_NONE, KEY_SW_C}, {MODIFIER_NONE, KEY_SW_R}, {MODIFIER_NONE, KEY_SW_L}, {MODIFIER_NONE, KEY_LAYER3}, //leftmost thumb
  {MODIFIER_NONE, KEY_SW_A}, {MODIFIER_NONE, KEY_SW_O}, {MODIFIER_NONE, KEY_SW_E}, {MODIFIER_NONE, KEY_SW_U}, {MODIFIER_NONE, KEY_SW_I}, {MODIFIER_NONE, KEY_SW_SPACE},
    {MODIFIER_NONE, KEY_SW_D}, {MODIFIER_NONE, KEY_SW_H}, {MODIFIER_NONE, KEY_SW_T}, {MODIFIER_NONE, KEY_SW_N}, {MODIFIER_NONE, KEY_SW_S}, {MODIFIER_LGUI, 0},
  {MODIFIER_LSHIFT, KEY_SW_COMMA}, {MODIFIER_NONE, KEY_SW_Q}, {MODIFIER_NONE, KEY_SW_J}, {MODIFIER_NONE, KEY_SW_K}, {MODIFIER_NONE, KEY_SW_X}, {MODIFIER_NONE, KEY_LAYER1},
    {MODIFIER_NONE, KEY_SW_B}, {MODIFIER_NONE, KEY_SW_M}, {MODIFIER_NONE, KEY_SW_W}, {MODIFIER_NONE, KEY_SW_V}, {MODIFIER_NONE, KEY_SW_Z}, {MODIFIER_NONE, KEY_LAYER2}, //rightmost thumb
  {MODIFIER_NONE, 0}, {MODIFIER_NONE, KEY_SW_LEFT}, {MODIFIER_NONE, KEY_SW_UP}, {MODIFIER_NONE, KEY_SW_RIGHT}, {0, 0}, {MODIFIER_NONE, KEY_SW_DOWN},
    {MODIFIER_NONE, 0}, {MODIFIER_LALT, 0}, {MODIFIER_NONE, KEY_SW_PGUP}, {MODIFIER_NONE, KEY_SW_END}, {0, 0}, {MODIFIER_NONE, KEY_SW_PGDOWN},
  //level 1
  {MODIFIER_ALTGR, KEY_SW_DDOT}, {MODIFIER_NONE, KEY_SW_7}, {MODIFIER_NONE, KEY_SW_8},{MODIFIER_NONE, KEY_SW_9}, {MODIFIER_LSHIFT, KEY_SW_7}, {MODIFIER_LCTRL, 0}, 
    {MODIFIER_NONE, KEY_SW_ESC}, {MODIFIER_ALTGR, KEY_SW_7} ,{MODIFIER_ALTGR, KEY_SW_0}, {MODIFIER_ALTGR, KEY_SW_4}, {MODIFIER_NONE, KEY_SW_AO}, {0, 0},
  {MODIFIER_NONE, KEY_SW_TAB}, {MODIFIER_NONE, KEY_SW_4}, {MODIFIER_NONE, KEY_SW_5}, {MODIFIER_NONE, KEY_SW_6}, {MODIFIER_NONE, KEY_SW_ENTER}, {MODIFIER_NONE, KEY_SW_SPACE}, 
    {MODIFIER_NONE, KEY_SW_DELETE}, {MODIFIER_LSHIFT, KEY_SW_8}, {MODIFIER_LSHIFT, KEY_SW_9}, {MODIFIER_NONE, KEY_SW_AE}, {MODIFIER_NONE, KEY_SW_OE}, {MODIFIER_LGUI, 0},
  {MODIFIER_NONE, KEY_SW_BACKSPACE}, {MODIFIER_NONE, KEY_SW_1}, {MODIFIER_NONE, KEY_SW_2}, {MODIFIER_NONE, KEY_SW_3}, {MODIFIER_NONE, KEY_SW_0}, {0, KEY_LAYER1},
    {MODIFIER_ALTGR, KEY_SW_PLUS}, {MODIFIER_ALTGR, KEY_SW_8}, {MODIFIER_ALTGR, KEY_SW_9}, {MODIFIER_LSHIFT, KEY_SW_1}, {MODIFIER_ALTGR, KEY_SW_LESST}, {MODIFIER_LSHIFT, 0},
  {MODIFIER_NONE, 0}, {MODIFIER_NONE, KEY_SW_LEFT}, {MODIFIER_NONE, KEY_SW_UP}, {MODIFIER_LSHIFT, KEY_SW_5}, {0, 0}, {MODIFIER_NONE, KEY_SW_DOWN}, 
    {MODIFIER_NONE, 0}, {MODIFIER_LALT, 0}, {MODIFIER_NONE, KEY_SW_PGUP}, {MODIFIER_NONE, KEY_SW_END}, {0, 0}, {MODIFIER_NONE, KEY_SW_PGDOWN},

  //level 2
  {0, KEY_SW_F2}, {0, KEY_SW_F3}, {0, KEY_SW_F4}, {0, KEY_SW_F5}, {0, KEY_SW_F6}, {MODIFIER_LCTRL, 0}, 
    {0, KEY_SW_F7}, {0, KEY_SW_F8}, {0, KEY_SW_F9}, {0, KEY_SW_F10}, {0, KEY_SW_F11}, {0, 0},
   {0, KEY_SW_PLUS}, {0, KEY_SW_HYPH}, {MODIFIER_LSHIFT, KEY_SW_0}, {MODIFIER_LSHIFT, KEY_SW_QUOTE},{0, KEY_SW_F1}, {0, 0}, 
    {0, KEY_SW_F12}, {MODIFIER_ALTGR, KEY_SW_2}, {MODIFIER_LSHIFT, KEY_SW_PLUS}, {0, KEY_SW_ACCENT}, {MODIFIER_LSHIFT, KEY_SW_DDOT}, {MODIFIER_LGUI, 0},
  {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0},
    {0, 0}, {MODIFIER_LALT, 0}, {0, 0}, {0, 0}, {0, 0}, {0, KEY_LAYER2},
  {MODIFIER_NONE, 0}, {MODIFIER_NONE, KEY_SW_LEFT}, {MODIFIER_NONE, KEY_SW_UP}, {MODIFIER_NONE, KEY_SW_BACKSPACE}, {0, 0}, {MODIFIER_NONE, KEY_SW_DOWN}, 
    {MODIFIER_NONE, 0}, {MODIFIER_LALT, 0}, {MODIFIER_NONE, KEY_SW_PGUP}, {MODIFIER_NONE, KEY_SW_END}, {0, 0}, {MODIFIER_NONE, KEY_SW_PGDOWN},
    //level 3
  {MODIFIER_LSHIFT, KEY_SW_2}, {MODIFIER_NONE, KEY_SW_LESST}, {MODIFIER_LSHIFT, KEY_SW_LESST}, {MODIFIER_LSHIFT, KEY_SW_P}, {MODIFIER_LSHIFT, KEY_SW_Y}, {MODIFIER_LSHIFT, KEY_SW_ENTER},
    {MODIFIER_LSHIFT, KEY_SW_F}, {MODIFIER_LSHIFT, KEY_SW_G}, {MODIFIER_LSHIFT, KEY_SW_C}, {MODIFIER_LSHIFT, KEY_SW_R}, {MODIFIER_LSHIFT, KEY_SW_L}, {0, KEY_LAYER3},
  {MODIFIER_LSHIFT, KEY_SW_A}, {MODIFIER_LSHIFT, KEY_SW_O}, {MODIFIER_LSHIFT, KEY_SW_E}, {MODIFIER_LSHIFT, KEY_SW_U}, {MODIFIER_LSHIFT, KEY_SW_I}, {MODIFIER_NONE, KEY_SW_SPACE},
    {MODIFIER_LSHIFT, KEY_SW_D}, {MODIFIER_LSHIFT, KEY_SW_H}, {MODIFIER_LSHIFT, KEY_SW_T}, {MODIFIER_LSHIFT, KEY_SW_N}, {MODIFIER_LSHIFT, KEY_SW_S}, {0, 0},
  {MODIFIER_LSHIFT, KEY_SW_PUNCT}, {MODIFIER_LSHIFT, KEY_SW_Q}, {MODIFIER_LSHIFT, KEY_SW_J}, {MODIFIER_LSHIFT, KEY_SW_K}, {MODIFIER_LSHIFT, KEY_SW_X}, {MODIFIER_NONE, KEY_SW_BACKSPACE},
    {MODIFIER_LSHIFT, KEY_SW_B}, {MODIFIER_LSHIFT, KEY_SW_M}, {MODIFIER_LSHIFT, KEY_SW_W}, {MODIFIER_LSHIFT, KEY_SW_V}, {MODIFIER_LSHIFT, KEY_SW_Z}, {0, 0},
  {MODIFIER_NONE, 0}, {MODIFIER_NONE, KEY_SW_LEFT}, {MODIFIER_NONE, KEY_SW_UP}, {MODIFIER_NONE, KEY_SW_RIGHT}, {0, 0}, {MODIFIER_NONE, KEY_SW_DOWN}, 
    {MODIFIER_NONE, 0}, {MODIFIER_LALT, 0}, {MODIFIER_NONE, KEY_SW_PGUP}, {MODIFIER_NONE, KEY_SW_END}, {0, 0}, {MODIFIER_NONE, KEY_SW_PGDOWN}
};

key_status_t key_status[N_COLS*N_ROWS];
unsigned int modifier;

//out corresponds to rows 0-4
//in corresponds to columns 0-5
unsigned int local_key_address(int out, int in){
  return N_COLS*out+in;
}


//out corresponds to columns 5-11
//in corresponds to rows 0-4
unsigned int remote_key_address(int out, int in){
  return N_COLS*in + REMOTE_COL_OFFS + out;
}

unsigned int key_address(int offs, int layer){
  return layer*N_COLS*N_ROWS + offs;
}

void set_int_array(int *arr, size_t n_elems, int val){
  size_t i=0;
  for (;i<n_elems;++i){
    arr[i] = val;
  }  
}

void reset_key_statuses(){
  int i = 0;
  for (;i<N_COLS*N_ROWS; ++i){
    key_status[i] = {0, UNASSIGNED_UP};
  }
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
  for (;r<N_OUT_PINS;++r){
    pinMode(ROW_PINS[r], OUTPUT);
    digitalWrite(ROW_PINS[r], LOW);
  }

  int c = 0;
  for (;c<N_IN_PINS;++c){
    pinMode(COL_PINS[c], INPUT_PULLDOWN);
  }

  stack_init(&free_keys);
  stack_push(&free_keys, 5);
  stack_push(&free_keys, 4);
  stack_push(&free_keys, 3);
  stack_push(&free_keys, 2);
  stack_push(&free_keys, 1);
  stack_push(&free_keys, 0);
  
  i2c_init();
  mcp23018_init();
  
  reset_key_statuses();
  set_int_array(modifier_counters, N_MODIFIERS, 0);
  set_int_array(active_layers, N_MODIFIERS, 0);
  modifier=0;
  Serial.println("Finish Init");
}


void write_matrix_local(int row){
    int i=0;
    int val = HIGH;
    for (;i<N_OUT_PINS;++i)
    {
      if (i == row)
        val = HIGH;
      else
        val = LOW;
      digitalWrite(ROW_PINS[i], val);
    }

}

void write_matrix_remote(int output){
  uint8_t output_mask = 0xFF;
  bitClear(output_mask, output);

  mcp23018_write_reg(TARGET, GPIOA, output_mask);
}

void read_matrix_remote(int *out_buf){
  uint8_t res = mcp23018_read_reg(TARGET, GPIOB);
  int err = mcp23018_read_status();

  if (err){
    #ifdef DEBUG
      Serial.print("Error receiving: ");
      Serial.println(err);
    #endif
    return;
  }
      
  #ifdef DEBUG
    Serial.printf("received from %#02x\n", TARGET);
    Serial.print(res, BIN);
    Serial.println(" OK");
  #endif
  
  int i;
  for (;i<N_IN_REMOTES; ++i){
    //Because the ioexpander has open drain outputs,
    //Low means the key is pressed. Thus the bits need to be flipped
    out_buf[i] = (0 == bitRead(res,i));
  }
}

void read_matrix_local(int *out_buff){
  int col=0;
  int col_pin=0;
  for (; col<N_IN_PINS; ++col){
    col_pin = COL_PINS[col];
    out_buff[col] = digitalRead(col_pin);
  }
}


int get_free_key_nr(){
  if (stack_is_empty(&free_keys)){
    return -1;
  }

  int res = stack_pop(&free_keys);
  return res;
}

void return_key_nr(int key_nr){
  stack_push(&free_keys, key_nr);
}

void send_usb(){
  Keyboard.send_now();
}

key_status_t get_key_status(int key_idx){
  return key_status[key_address(key_idx, 0)];
}

void set_status(int key_idx, key_status_t new_status){
  key_status[key_address(key_idx, 0)] = new_status;
}

key_data_t get_key_data(int key_idx){
  return key_map[key_address(key_idx,fn_layer)];
}

bool should_update(int key_state, key_status_t old_status){

  if ( 
      //Now: pressed, before: unassigned 
      (key_state == HIGH && !old_status.state)
      //Now: up, before: pressed and assigned
      ||(key_state == LOW && old_status.state)
     ){
    return true;
  }

  return false;
}

//TODO MAKE NON USB_KEYS DONT COUNT TOWARDS PRESSED KEYS
bool set_usb_key(int usb_key, int value){
  if (usb_key >= 0 && usb_key <= MAX_N_USB_KEYS
        && value<HID_MAX){
    keyboard_keys[usb_key] = value;
    return true;
  }
  return false;
}

// Updates modifers
// More than one key per modifier is supported
// returns 0 if no change, and 1 if there is an update
key_status_t update_modifiers(int key_state,  key_status_t old_status, key_data_t key_data){
  uint8_t old_modifiers = g_modifiers;

  int counter = 0;
  int modifier = 0;

  if (key_state == HIGH){
    modifier = key_data.modifier;
    if (modifier == MODIFIER_NONE)
      return old_status;
    counter = modifier_counters[modifier]; 
    
    if (counter == 0){
      bitSet(g_modifiers, modifier);
      old_status.modifier = modifier;
    }
    
    modifier_counters[modifier] = counter + 1;
  }
  else if (key_state == LOW){
    modifier = old_status.modifier;
    if (modifier == MODIFIER_NONE)
      return old_status;
      
    counter = modifier_counters[modifier];
    if (counter == 1){
      bitClear(g_modifiers,modifier);
      old_status.modifier = MODIFIER_NONE;
    }
    modifier_counters[modifier] = counter - 1;
  }

  Keyboard.set_modifier(g_modifiers);

  return old_status;
}

/*
 * The currently activated function Layer with the highest number determines the layer
 * 
 */
key_status_t update_special_keys(int key_state, key_status_t old_status, key_data_t key_data){
  int key_value = key_data.value;

  int layer = key_value - KEY_LAYER1 + 1;
  if ( !(layer >= 0 && layer < N_FN_LAYERS) ){
    return old_status;
  }
  
  if (key_state == HIGH){
    active_layers[layer] = 1;
    Serial.print("LAYER PRESS ");
    Serial.println(layer);
    print_int_array(active_layers, N_FN_LAYERS);
    
    if (layer>fn_layer){
      Serial.print("Update");
      fn_layer = layer;
    }
  }
  else if (key_state == LOW){
    //if (!active_layers[layer])
    //  return;
      
    active_layers[layer] = 0;
    Serial.print("LAYER UNPRESS ");
    Serial.println(layer);
    print_int_array(active_layers, N_FN_LAYERS);

    int i = N_FN_LAYERS - 1;
    for (;i>=0; --i){
      if (active_layers[i]){
        fn_layer = i;
        break;
      }
      fn_layer = DEFAULT_FN_LAYER;
    }
  //TODO RESET MODIFIERS
  }

  return old_status;
}

key_status_t update_usb_keys(int key_state, key_status_t old_status, key_data_t key_data){
  if (key_data.value < HID_MIN or key_data.value > HID_MAX)
    return old_status; 

  int next_key_nr = 0;
  int usb_key_value = 0;
  
  if (key_state == HIGH){
    Serial.println("KEY DOWN");
    next_key_nr = get_free_key_nr();
    usb_key_value = key_data.value;
    ++n_pressed_keys;
    set_usb_key(next_key_nr, usb_key_value);
  }
  else{
    Serial.println("KEY UP");
    return_key_nr(old_status.usb_key);     
    next_key_nr = UNASSIGNED_UP;
    usb_key_value = HID_RELEASED;
    --n_pressed_keys;

    set_usb_key(old_status.usb_key, usb_key_value);
  }

  old_status.usb_key = next_key_nr;
  return old_status;
}

bool key_status_diff(key_status_t s1, key_status_t s2){
  return ( (s1.modifier != s2.modifier)
        || (s1.usb_key != s2.usb_key) );
}

void handle_key_press(int key_idx, int key_state){

      
      key_status_t old_status = get_key_status(key_idx); //-1, 0, 1, 2, 3, 4, 5
      key_data_t key_data = get_key_data(key_idx);
      
      if (should_update(key_state, old_status)){
        Serial.print("FN ");
        Serial.println(fn_layer);

        key_status_t new_status = update_usb_keys(key_state, old_status, key_data);
        new_status = update_modifiers(key_state, new_status, key_data);
        new_status = update_special_keys(key_state, new_status, key_data);
        
        new_status.state = key_state;
        key_status[key_idx] = new_status;
        
        if (key_status_diff(new_status, old_status)){
          send_usb();
        }
      }
}

void run_main(){
  //TODO CALCULATE PROPER MAX AT COMPILE TIME
  const size_t OUT_LEN = N_OUT_PINS + N_OUT_REMOTES;
  
  int out_buff[OUT_LEN] = {0};
  set_int_array(out_buff, OUT_LEN, LOW);
  
  int out = 0;

//  //Half with the teensy
  for (out=0;out<N_OUT_PINS;++out){
    write_matrix_local(out);
    read_matrix_local(out_buff);

    int in = 0;
    for (;in<N_IN_PINS;++in){
      int key_idx = local_key_address(out, in);
      int key_val = out_buff[in];
      handle_key_press(key_idx, key_val);
    }
  }

  //Half with the ioexpander
  for (out=0;out<N_OUT_REMOTES;++out){
    write_matrix_remote(out);
    read_matrix_remote(out_buff);
  
    int in = 0;
    for (;in<N_IN_REMOTES;++in){
      int key_idx = remote_key_address(out, in);
      int key_val = out_buff[in];
      handle_key_press(key_idx, key_val);
    }
  }
}
