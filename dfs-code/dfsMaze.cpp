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

double distances[4];
char direction = 'N';

mutex_t mutex;
Sensor S;
Motor M(distances, &mutex);

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
  M.turn(1);
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
  M.turn(180);

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
  M.turn(90);
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
    mutex_enter_blocking(&mutex);
    leftDistance = distances[0];
    frontleft = distances[1];
    frontright = distances[2];
    rightDistance = distances[3];
    mutex_exit(&mutex);

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
      cout << "front pos: " << front_pos.x << " , " << front_pos.y << endl;

      MoveForward(x, y, direction);
      path.addpathEnd(front_pos);
      visited.addEnd(front_pos);

      cout << "Moving forward" << endl;
    }

    else if (leftDistance > 30 && !visited.contains(left_pos))
    {
      cout << "left pos: " << left_pos.x << " , " << left_pos.y << endl;
      flag = 1;
      direction = TurnLeft(direction);
      cout << "direction: " << direction << endl;

      cout << "Turning Left" << endl;
    }

    else if (rightDistance > 30 && !visited.contains(right_pos))
    {
      cout << "right pos: " << right_pos.x << " , " << right_pos.y << endl;
      flag = 1;
      direction = TurnRight(direction);

      cout << "Turning Right" << endl;
    }

    else
    {

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

        if (currentNode.equals(targetNode))
        {
          nodes.removeEnd();
          flag = 0;
          break;
        }

        else if (front_pos.equals(previousNode))
        {
          MoveForward(x, y, direction);
          path.removepathEnd();
        }

        else if (back_pos.equals(previousNode))
        {
          direction = Uturn(direction);

          MoveForward(x, y, direction);
          path.removepathEnd();
        }

        else if (left_pos.equals(previousNode))
        {
          direction = TurnLeft(direction);
        }

        else if (right_pos.equals(previousNode))
        {
          direction = TurnRight(direction);
        }

        // else // Buzzer
        // {
        //   M.turn(3, 1);
        // }
        if (path.isEmpty() || nodes.isEmpty())
        {
          // cout << " Backtracking terminated. Path or Nodes list is empty." << endl;
          // M.turn(3, 1);
          break;
        }
      }
    }
  }
}

/////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////

void core1main()
{
  while (1)
  {
    S.readings(distances);
  }
}

int main()
{
  stdio_init_all();
  mutex_init(&mutex);
  sleep_ms(2000);

  PIO pio = pio0;
  uint offset = pio_add_program(pio, &blink_program);
  blink_pin_forever(pio, 0, offset, PICO_DEFAULT_LED_PIN, 1);

  multicore_launch_core1(core1main);

  // Initialize I2C on SDA (Pin 8), SCL (Pin 9)
    i2c_init(i2c_default, 100 * 1000);
    gpio_set_function(8, GPIO_FUNC_I2C); // SDA
    gpio_set_function(9, GPIO_FUNC_I2C); // SCL
    gpio_pull_up(8);
    gpio_pull_up(9);

    // Initialize Compass
    QMC5883LCompass compass;
    compass.init();

    // Set magnetic declination to 0° 39' E
    compass.setMagneticDeclination(0, 39);

    printf("Starting azimuth readings...\n");

    while (true) {
        compass.read();
        int x = compass.getX();
        int y = compass.getY();
        int z = compass.getZ();
        float azimuth = compass.getAzimuth();

        // printf("X: %d, Y: %d, Z: %d\n",x,y,z);
        // Print azimuth value
        printf("Azimuth: %f degrees\n", azimuth);

        sleep_ms(100);  // Delay between readings
    }

  // QMC5883LCompass compass;
  // compass.init();

  // compass.setMagneticDeclination(0, 39);

  // while (1)
  // {
  //   i2c_scan();
  //   compass.read();
  //   int azimuth = compass.getAzimuth();

  //   cout << compass.getAzimuth() << endl;
  //   sleep_ms(500);
  // }
  // while(1){
  //   M.move_forward(20);
  //   sleep_ms(1000);
  // }

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
