typedef struct key_data_t{
  int modifier;
  int hid_code;
}key_data_t;

typedef struct key_status_t{
  //The current effect the key has on the modifiers
  int modifier;
  //The index in the reported usb_keys if it has been been previously assigned
  //UNASSIGNED_USB_KEY otherwise.
  int usb_key;
} key_status_t;


class int_stack{
private:
  static const int MAX_LEN = 48;
  int head;
  const int EMPTY = -1;
  int arr[MAX_LEN];

public:

  bool empty(){
    return head == EMPTY;
  }
    
  void push(int val){
    if (head < MAX_LEN){
      ++head;
      arr[head] = val;
    }
  }
  
  int pop(){
    if (empty())
      return EMPTY;
    
    int val = arr[head];
    --head;
    return val;
  }

};

void init_main();
unsigned int key_addr(int x, int y, int l);
void run_main();
