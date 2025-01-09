#include <stdio.h>
#include <iostream>
#include <vector>
#include "pico/stdlib.h"
#include "pico/multicore.h"

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
Motor M(distances);

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

char TurnLeft(char dir)
{
  M.turn(1, 0);
  if (dir == 'N')
    return 'W';
  if (dir == 'W')
    return 'S';
  if (dir == 'S')
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

char TurnRight(char dir)
{
  M.turn(1, 1);
  if (dir == 'N')
    return 'E';
  if (dir == 'E')
    return 'S';
  if (dir == 'S')
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
  if (frontdistance > 15)
  {
    count++;
  }
  if (leftdistance > 15)
  {
    count++;
  }
  if (rightdistance > 15)
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
void core_1_func()
{
  while (1)
  {
    mutex_enter_blocking(&mutex);
    S.readings(distances);
    mutex_exit(&mutex);

    sleep_ms(300);
  }
}

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

  while (true)
  {

    int backtrackingSteps = 0;
    const int MAX_BACKTRACKING_STEPS = 100;

    // path.printNodes();
    // nodes.printNodes();

    for (int i = 0; i < 4; i++)
      std::cout << (distances[i]) << " ";
    cout << endl;

    if (distances[0] == -1 && distances[1] == -1 && distances[2] == -1 && distances[3] == -1)
    {
      // buzzer
      //  cout << "Solved!" << endl;
      break;
    }

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

    if (frontDistance > 15 && !visited.contains(front_pos))
    {
      flag = 0;
      MoveForward(x, y, direction);
      path.addpathEnd(front_pos);
      visited.addEnd(front_pos);

      // cout << "Moving forward" << endl;
    }

    else if (leftDistance > 15 && !visited.contains(left_pos))
    {
      flag = 1;
      direction = TurnLeft(direction);
      // cout << "Turning Left" << endl;
    }

    else if (rightDistance > 15 && !visited.contains(right_pos))
    {
      flag = 1;
      direction = TurnRight(direction);
      // cout << "Turning Right" << endl;
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

          // cout << "Moving forward to the previous node" << endl;
        }

        else if (back_pos.equals(previousNode))
        {
          direction = Uturn(direction);
          MoveForward(x, y, direction);
          path.removepathEnd();
          // cout << "Taking a U-turn and moving to previous node" << endl;
        }

        else if (left_pos.equals(previousNode))
        {
          direction = TurnLeft(direction);
          // cout << "Turning Left" << endl;
        }

        else if (right_pos.equals(previousNode))
        {
          direction = TurnRight(direction);
          // cout << "Turning Right" << endl;
        }

        else // Buzzer
        {
          // cout << "Error in backtracking\n";
        }
        if (path.isEmpty() || nodes.isEmpty())
        {
          // cout << " Backtracking terminated. Path or Nodes list is empty." << endl;
          break;
        }
      }
    }
  }
}

int main()
{
  stdio_init_all();
  mutex_init(&mutex);
  multicore_reset_core1();
  multicore_launch_core1(core_1_func);

  PIO pio = pio0;
  uint offset = pio_add_program(pio, &blink_program);
  blink_pin_forever(pio, 0, offset, PICO_DEFAULT_LED_PIN, 1);
  // blink, doesnt use cpu

  int arr[4] = {0, 0, 0, 0};
  // while (1)
  // {
  //   S.readings(arr);
  //   printf("%d-%d-%d-%d\n", arr[0], arr[1], arr[2], arr[3]);
  // }
  DFS();
}

// infinity - 8190