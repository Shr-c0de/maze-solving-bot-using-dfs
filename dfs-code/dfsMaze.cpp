#include <stdio.h>
#include <iostream>
#include <vector>
#include <math.h>
#include <cmath>
#include "pico/stdlib.h"
#include "pico/multicore.h"
// #include "pico/math.h"
#include "hardware/i2c.h"
#include "hardware/pio.h"
// pico libraries

using namespace std;

#include "blink.pio.h"
#include "VL53L0X.h"
#include "motors.h"
#include "Sensors.h"
// user libraries

#define I2C_PORT i2c0
#define I2C_SDA 8
#define I2C_SCL 9
// i2c pin definitons

void blink_pin_forever(PIO pio, uint sm, uint offset, uint pin, uint freq)
{
  blink_program_init(pio, sm, offset, pin);
  pio_sm_set_enabled(pio, sm, true);

  printf("Blinking pin %d at %d Hz\n", pin, freq);

  // PIO counter program takes 3 more cycles in total than we pass as
  // input (wait for n + 1; mov; jmp)
  pio->txf[sm] = (125000000 / (2 * freq)) - 3;
}

mutex_t mutex;
Sensor S;
double distances[4];
Motor M;

class Node
{
public:
  int x, y;
  Node(int x = 0, int y = 0)
      : x(x), y(y) {}

  bool equals(const Node &other) const
  {
    return x == other.x && y == other.y;
  }
};

class ListNode
{
public:
  Node data;
  ListNode *next;
  ListNode *prev;
  ListNode(Node node)
      : data(node), next(nullptr), prev(nullptr) {}
};

class LinkedList
{
public:
  ListNode *head;
  ListNode *tail;
  int elements;

  LinkedList()
      : head(nullptr), tail(nullptr), elements(0) {}

  void addEnd(Node node)
  {
    ListNode *newNode = new ListNode(node);
    if (tail == nullptr)
    {
      head = tail = newNode;
    }
    else
    {
      tail->next = newNode;
      newNode->prev = tail;
      tail = newNode;
    }
  }

  void addpathEnd(Node node)
  {
    ListNode *newNode = new ListNode(node);
    if (tail == nullptr)
    {
      head = tail = newNode;
    }
    else
    {
      tail->next = newNode;
      newNode->prev = tail;
      tail = newNode;
    }
    elements++;
  }

  void removeEnd()
  {
    if (tail != nullptr)
    {
      ListNode *temp = tail;
      tail = tail->prev;
      if (tail != nullptr)
      {
        tail->next = nullptr;
      }
      else
      {
        head = nullptr;
      }
      delete temp;
    }
  }

  void removepathEnd()
  {
    if (elements == 1)
    {
      if (tail != nullptr)
      {
        ListNode *temp = tail;
        tail = tail->prev;
        if (tail != nullptr)
        {
          tail->next = nullptr;
        }
        else
        {
          head = nullptr;
        }
        delete temp;

        LinkedList path;
        Node initial(0, 0);
        path.addpathEnd(initial);
      }
    }
    else if (tail != nullptr)
    {
      ListNode *temp = tail;
      tail = tail->prev;
      if (tail != nullptr)
      {
        tail->next = nullptr;
      }
      else
      {
        head = nullptr;
      }
      delete temp;
    }
    elements--;
  }

  Node getEnd()
  {
    return (tail != nullptr) ? tail->data : Node(-1, -1);
  }

  bool isEmpty()
  {
    return tail == nullptr;
  }

  bool contains(Node n)
  {
    ListNode *current = head;
    while (current != nullptr)
    {
      if (current->data.equals(n))
      {
        return true;
      }
      current = current->next;
    }
    return false;
  }

  /*
  void printNodes()
  {
    ListNode *current = head;
    cout << "Current Nodes: ";
    while (current != nullptr)
    {
      cout << "(" << current->data.x << ", " << current->data.y << ") ";
      current = current->next;
    }
    cout << endl;
  }
  */

  Node getPreviousNode()
  {
    if (tail != nullptr && tail->prev != nullptr)
    {
      return tail->prev->data;
    }
    return Node(-1, -1);
  }
};

char TurnLeft(char direction)
{
  M.turn(1, 0);
  if (direction == 'N')
    return 'W';
  if (direction == 'W')
    return 'S';
  if (direction == 'S')
    return 'E';
  return 'N';
}

char Uturn(char dir)
{
  M.turn(1, 0);
  sleep_ms(100);
  M.turn(1, 0);

  if (dir == 'N')
    return 'S';
  if (dir == 'W')
    return 'E';
  if (dir == 'S')
    return 'N';
  return 'W';
}

char TurnRight(char direction)
{
  M.turn(1, 1);
  if (direction == 'N')
    return 'E';
  if (direction == 'E')
    return 'S';
  if (direction == 'S')
    return 'W';
  return 'N';
}

Node MoveForward(int &x, int &y, char direction)
{
  M.move_forward(1);

  if (direction == 'N')
    y++;
  else if (direction == 'S')
    y--;
  else if (direction == 'E')
    x++;
  else if (direction == 'W')
    x--;

  return Node(x, y);
}

int checknodes(int frontdistance, int leftdistance, int rightdistance)
{
  int count = 0;
  if (frontdistance > 25)
  {
    count++;
  }
  if (leftdistance > 25)
  {
    count++;
  }
  if (rightdistance > 25)
  {
    count++;
  }

  if (count > 1)
  {
    cout << "It is a node" << endl;
    return 1;
  }
  return 0;
}

// Left to define
int frontdistance(int front1Distance, int front2Distance)
{
  return max(front1Distance, front2Distance);
}

LinkedList path;
LinkedList visited;
LinkedList nodes;

char direction = 'N';

//

//

void DFS()
{
  int x = 0, y = 0;
  Node start(x, y);
  path.addpathEnd(start);
  visited.addEnd(start);
  nodes.addEnd(start);
  int flag = 0;

  int leftDistance, frontleft, frontright, rightDistance;
  sleep_ms(2000);

  while (true)
  {
    S.readings(distances);
    printf("DFS: %f, %f, %f, %f\n", distances[0], distances[1], distances[2], distances[3]);

    // int backtrackingSteps = 0;
    // const int MAX_BACKTRACKING_STEPS = 100;

    // path.printNodes();
    // nodes.printNodes();
    // mutex_enter_blocking(&mutex);

    leftDistance = distances[0];
    frontleft = distances[1];
    frontright = distances[2];
    rightDistance = distances[3];

    // mutex_exit(&mu//tex);

    int frontDistance = frontdistance(frontleft, frontright);

    Node front_pos(x, y);
    if (direction == 'N')
      front_pos.y++;
    else if (direction == 'S')
      front_pos.y--;
    else if (direction == 'E')
      front_pos.x++;
    else if (direction == 'W')
      front_pos.x--;

    Node left_pos(x, y);
    if (direction == 'N')
      left_pos.x--;
    else if (direction == 'S')
      left_pos.x++;
    else if (direction == 'E')
      left_pos.y++;
    else if (direction == 'W')
      left_pos.y--;

    Node right_pos(x, y);
    if (direction == 'N')
      right_pos.x++;
    else if (direction == 'S')
      right_pos.x--;
    else if (direction == 'E')
      right_pos.y--;
    else if (direction == 'W')
      right_pos.y++;

    Node back_pos(x, y);
    if (direction == 'N')
      back_pos.y--;
    else if (direction == 'S')
      back_pos.y++;
    else if (direction == 'E')
      back_pos.x--;
    else if (direction == 'W')
      back_pos.x++;

    Node current_pos(x, y);

    if (!flag)
    {

      int is_node = checknodes(frontDistance, leftDistance, rightDistance);
      if (is_node)
      {
        nodes.addEnd(current_pos);
      }
    }

    
    if (frontDistance > 20 && !visited.contains(front_pos))
    {
      flag = 0;
      cout<< "front pos: "<<front_pos.x <<" , "<<front_pos.y<< endl;

      MoveForward(x, y, direction);
      path.addpathEnd(front_pos);
      visited.addEnd(front_pos);
      // M.move_forward(1);

      cout << "Moving forward" << endl;
    }


    else if (leftDistance > 30 && !visited.contains(left_pos))
    {
      cout<< "left pos: "<<left_pos.x <<" , "<<left_pos.y<< endl;
       flag = 1;
      direction = TurnLeft(direction);
      cout<<"direction: "<<direction<<endl;
      // M.turn(1,0);
      
      cout << "Turning Left" << endl;
    }

    else if (rightDistance > 30 && !visited.contains(right_pos))
    {
      cout<< "right pos: "<<right_pos.x <<" , "<<right_pos.y<< endl;
       flag = 1;
      direction = TurnRight(direction);
      // M.turn(1,1);

      cout << "Turning Right" << endl;
    }

    else
    {

      // cout << "Getting into backtracking" << endl;

      Node targetNode;

      int is_a_node = checknodes(frontDistance, leftDistance, rightDistance);

      if (is_a_node)
      {
        targetNode = nodes.getPreviousNode();
      }
      else
      {
        targetNode = nodes.getEnd();
      }

      while (true)
      {
        /*
        backtrackingSteps++;
        if (backtrackingSteps > MAX_BACKTRACKING_STEPS)
        {
          cout << " Backtracking exceeded step limit." << endl;
          break;
        }
        */

        Node currentNode = path.getEnd();
        x = currentNode.x;
        y = currentNode.y;
        Node previousNode = path.getPreviousNode();

        front_pos = Node(x, y);
        if (direction == 'N')
          front_pos.y++;
        else if (direction == 'S')
          front_pos.y--;
        else if (direction == 'E')
          front_pos.x++;
        else if (direction == 'W')
          front_pos.x--;

        left_pos = Node(x, y);
        if (direction == 'N')
          left_pos.x--;
        else if (direction == 'S')
          left_pos.x++;
        else if (direction == 'E')
          left_pos.y++;
        else if (direction == 'W')
          left_pos.y--;

        right_pos = Node(x, y);
        if (direction == 'N')
          right_pos.x++;
        else if (direction == 'S')
          right_pos.x--;
        else if (direction == 'E')
          right_pos.y--;
        else if (direction == 'W')
          right_pos.y++;

        back_pos = Node(x, y);
        if (direction == 'N')
          back_pos.y--;
        else if (direction == 'S')
          back_pos.y++;
        else if (direction == 'E')
          back_pos.x--;
        else if (direction == 'W')
          back_pos.x++;

        // path.printNodes();
        // nodes.printNodes();

        if (currentNode.equals(targetNode))
        {
          // cout << "Reached the target node during backtracking" << endl;
          nodes.removeEnd();
          flag = 0;
          break;
        }

        else if (front_pos.equals(previousNode))
        {
          MoveForward(x, y, direction);
          path.removepathEnd();
          //M.move_forward(1);

          // cout << "Moving forward to the previous node" << endl;
        }

        else if (back_pos.equals(previousNode))
        {
          direction = Uturn(direction);
          M.turn(1,1);
          M.turn(1,1);

          MoveForward(x, y, direction);
          path.removepathEnd();
          //M.move_forward(1);
          // cout << "Taking a U-turn and moving to previous node" << endl;
        }

        else if (left_pos.equals(previousNode))
        {
          direction = TurnLeft(direction);
          //M.turn(1,0);
          // cout << "Turning Left" << endl;
        }

        else if (right_pos.equals(previousNode))
        {
          direction = TurnRight(direction);
          //M.turn(1,1);
          // cout << "Turning Right" << endl;
        }

        else // Buzzer
        {
          // cout << "Error in backtracking\n";
          M.turn(3,1);
        }
        if (path.isEmpty() || nodes.isEmpty())
        {
          // cout << " Backtracking terminated. Path or Nodes list is empty." << endl;
          M.turn(3,1);
          break;
        }
      }
    }
  }
}

/////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////




// QMC5883L Constants
#define QMC5883L_ADDR 0x0D
#define CONTROL_REG 0x09
#define DATA_REG 0x00
#define SET_RESET_REG 0x0B

class QMC5883LCompass {
private:
    uint8_t _ADDR;
    float _magneticDeclinationDegrees = 0.0;
    float _offset[3] = {0.0, 0.0, 0.0};
    float _scale[3] = {1.0, 1.0, 1.0};
    int16_t _vRaw[3] = {0, 0, 0};
    float _vCalibrated[3] = {0.0, 0.0, 0.0};

    void _writeReg(uint8_t reg, uint8_t value) {
        uint8_t data[2] = {reg, value};
        i2c_write_blocking(i2c_default, _ADDR, data, 2, false);
    }

    void _applyCalibration() {
        for (int i = 0; i < 3; i++) {
            _vCalibrated[i] = (_vRaw[i] - _offset[i]) * _scale[i];
        }
    }

public:
    QMC5883LCompass(uint8_t address = QMC5883L_ADDR) : _ADDR(address) {}

    void init() {
        _writeReg(SET_RESET_REG, 0x01); // Set Reset Register
        setMode(0x01, 0x0C, 0x10, 0x00); // Default mode configuration
    }

    void setMode(uint8_t mode, uint8_t odr, uint8_t rng, uint8_t osr) {
        _writeReg(CONTROL_REG, mode | odr | rng | osr);
    }

    void setMagneticDeclination(int degrees, uint8_t minutes) {
        _magneticDeclinationDegrees = degrees + minutes / 60.0f;
    }

    void read() {
        uint8_t data[6];
        uint8_t reg = DATA_REG;
        i2c_write_blocking(i2c_default, _ADDR, &reg, 1, true);
        i2c_read_blocking(i2c_default, _ADDR, data, 6, false);

        _vRaw[0] = (int16_t)(data[1] << 8 | data[0]);
        _vRaw[1] = (int16_t)(data[3] << 8 | data[2]);
        _vRaw[2] = (int16_t)(data[5] << 8 | data[4]);

        _applyCalibration();
    }

    int getX() {
        return (int)_vCalibrated[0];
    }

    int getY() {
        return (int)_vCalibrated[1];
    }

    int getZ() {
        return (int)_vCalibrated[2];
    }

    int getAzimuth() {
        float heading = atan2(_vCalibrated[1], _vCalibrated[0]) * 180.0 / M_PI;
        heading += _magneticDeclinationDegrees;
        if (heading < 0) heading += 360;
        if (heading >= 360) heading -= 360;
        return (int)heading;
    }
};



int main() {
    stdio_init_all();
    mutex_init(&mutex);
    sleep_ms(2000);

    PIO pio = pio0;
    uint offset = pio_add_program(pio, &blink_program);
    blink_pin_forever(pio, 0, offset, PICO_DEFAULT_LED_PIN, 1);

    // Initialize 
    i2c_init(i2c_default, 100 * 1000);
    gpio_set_function(8, GPIO_FUNC_I2C); // SDA
    gpio_set_function(9, GPIO_FUNC_I2C); // SCL
    gpio_pull_up(8);
    gpio_pull_up(9);

    QMC5883LCompass compass;
    compass.init();

    compass.setMagneticDeclination(0, 39);

    uint32_t start_time = time_us_32(); 
    uint32_t elapsed_time = 0;

    while (elapsed_time < 100000000) {
        compass.read();
        int azimuth = compass.getAzimuth();

        printf("Azimuth: %d degrees\n", azimuth);
        sleep_ms(50); 

        elapsed_time = time_us_32() - start_time;  
    }
   

    return 0;
}









  
  // blink, doesnt use cpu
  // while (1)
  // {
  //   printf("%f, %f, %f, %f\n", distances[0], distances[1], distances[2], distances[3]);
  //   sleep_ms(200);
  // }

  // while (1)
  // {
  //   M.move_forward(3);
  //   sleep_ms(2000);
  //   M.turn(1, 0);
  //   sleep_ms(2000);
  //   M.turn(1, 1);
  //   sleep_ms(2000);
  // }
  // DFS();


// infinity - 8190