#include <iostream>

using namespace std;

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

  Node getPreviousNode()
  {
    if (tail != nullptr && tail->prev != nullptr)
    {
      return tail->prev->data;
    }
    return Node(-1, -1);
  }
};

string TurnLeft(string dir)
{
  if (dir == "N")
    return "W";
  if (dir == "W")
    return "S";
  if (dir == "S")
    return "E";
  return "N";
}

string Uturn(string dir)
{
  if (dir == "N")
    return "S";
  if (dir == "W")
    return "E";
  if (dir == "S")
    return "N";
  return "W";
}

string TurnRight(string dir)
{
  if (dir == "N")
    return "E";
  if (dir == "E")
    return "S";
  if (dir == "S")
    return "W";
  return "N";
}

Node MoveForward(int &x, int &y, string direction)
{
  if (direction == "N")
    y++;
  else if (direction == "S")
    y--;
  else if (direction == "E")
    x++;
  else if (direction == "W")
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

LinkedList path;
LinkedList visited;
LinkedList nodes;

string direction = "N";
double distances[4];

void DFS()
{
  int x = 0, y = 0;
  Node start(x, y);
  path.addpathEnd(start);
  visited.addEnd(start);
  nodes.addEnd(start);
  int flag = 0;

  while (true)
  {

    int backtrackingSteps = 0;
    const int MAX_BACKTRACKING_STEPS = 100;

    path.printNodes();
    nodes.printNodes();

    cout << "Enter front1 distance: ";
    cin >> distances[0];
    cout << "Enter front2 distance: ";
    cin >> distances[1];
    cout << "Enter left distance: ";
    cin >> distances[2];
    cout << "Enter right distance: ";
    cin >> distances[3];

    if (distances[0] == -1 && distances[1] == -1 && distances[2] == -1 && distances[3] == -1)
    {
      cout << "Solved!" << endl;
      break;
    }

    int front1Distance = distances[0];
    int front2Distance = distances[1];
    int leftDistance = distances[2];
    int rightDistance = distances[3];

    int frontDistance = (front1Distance + front2Distance) / 2;

    Node front_pos(x, y);
    if (direction == "N")
      front_pos.y++;
    else if (direction == "S")
      front_pos.y--;
    else if (direction == "E")
      front_pos.x++;
    else if (direction == "W")
      front_pos.x--;

    Node left_pos(x, y);
    if (direction == "N")
      left_pos.x--;
    else if (direction == "S")
      left_pos.x++;
    else if (direction == "E")
      left_pos.y++;
    else if (direction == "W")
      left_pos.y--;

    Node right_pos(x, y);
    if (direction == "N")
      right_pos.x++;
    else if (direction == "S")
      right_pos.x--;
    else if (direction == "E")
      right_pos.y--;
    else if (direction == "W")
      right_pos.y++;

    Node back_pos(x, y);
    if (direction == "N")
      back_pos.y--;
    else if (direction == "S")
      back_pos.y++;
    else if (direction == "E")
      back_pos.x--;
    else if (direction == "W")
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

      cout << "Moving forward" << endl;
    }

    else if (leftDistance > 15 && !visited.contains(left_pos))
    {
      flag = 1;
      direction = TurnLeft(direction);
      cout << "Turning Left" << endl;
    }

    else if (rightDistance > 15 && !visited.contains(right_pos))
    {
      flag = 1;
      direction = TurnRight(direction);
      cout << "Turning Right" << endl;
    }

    else
    {

      cout << "Getting into backtracking" << endl;

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
        backtrackingSteps++;
        if (backtrackingSteps > MAX_BACKTRACKING_STEPS)
        {
          cout << " Backtracking exceeded step limit." << endl;
          break;
        }

        Node currentNode = path.getEnd();
        x = currentNode.x;
        y = currentNode.y;
        Node previousNode = path.getPreviousNode();

        front_pos = Node(x, y);
        if (direction == "N")
          front_pos.y++;
        else if (direction == "S")
          front_pos.y--;
        else if (direction == "E")
          front_pos.x++;
        else if (direction == "W")
          front_pos.x--;

        left_pos = Node(x, y);
        if (direction == "N")
          left_pos.x--;
        else if (direction == "S")
          left_pos.x++;
        else if (direction == "E")
          left_pos.y++;
        else if (direction == "W")
          left_pos.y--;

        right_pos = Node(x, y);
        if (direction == "N")
          right_pos.x++;
        else if (direction == "S")
          right_pos.x--;
        else if (direction == "E")
          right_pos.y--;
        else if (direction == "W")
          right_pos.y++;

        back_pos = Node(x, y);
        if (direction == "N")
          back_pos.y--;
        else if (direction == "S")
          back_pos.y++;
        else if (direction == "E")
          back_pos.x--;
        else if (direction == "W")
          back_pos.x++;

        path.printNodes();
        nodes.printNodes();

        if (currentNode.equals(targetNode))
        {
          cout << "Reached the target node during backtracking" << endl;
          nodes.removeEnd();
          flag = 0;
          break;
        }

        else if (front_pos.equals(previousNode))
        {
          MoveForward(x, y, direction);
          path.removepathEnd();
          cout << "Moving forward to the previous node" << endl;
        }

        else if (back_pos.equals(previousNode))
        {
          direction = Uturn(direction);
          MoveForward(x, y, direction);
          path.removepathEnd();
          cout << "Taking a U-turn and moving to previous node" << endl;
        }

        else if (left_pos.equals(previousNode))
        {
          direction = TurnLeft(direction);
          cout << "Turning Left" << endl;
        }

        else if (right_pos.equals(previousNode))
        {
          direction = TurnRight(direction);
          cout << "Turning Right" << endl;
        }
        
        else
        {
          cout << "Error in backtracking\n";
        }
        if (path.isEmpty() || nodes.isEmpty())
        {
          cout << " Backtracking terminated. Path or Nodes list is empty." << endl;
          break;
        }
      }
    }
  }
}

class ActionNode
{
public:
  string action;
  int steps;
  ActionNode *next;

  ActionNode(string action, int steps = 0)
      : action(action), steps(steps), next(nullptr) {}
};

class ActionList
{
public:
  ActionNode *head;
  ActionNode *tail;

  ActionList() : head(nullptr), tail(nullptr) {}

  void addAction(string action, int steps = 0)
  {
    ActionNode *newNode = new ActionNode(action, steps);
    if (tail == nullptr)
    {
      head = tail = newNode;
    }
    else
    {
      tail->next = newNode;
      tail = newNode;
    }
  }

  void printActions()
  {
    ActionNode *current = head;
    cout << "Final route: " << endl;
    while (current != nullptr)
    {
      if (current->action == "Move Forward")
      {
        cout << current->action << " " << current->steps << " steps" << endl;
      }
      else
      {
        cout << current->action << endl;
      }
      current = current->next;
    }
  }
};

ActionList finalpath(LinkedList &path)
{
  ActionList actions;

  ListNode *current = path.head;
  string currentDirection = "N";
  int stepCount = 0;

  while (current->next != nullptr)
  {
    Node currNode = current->data;
    Node nextNode = current->next->data;

    string newDirection;
    if (nextNode.x > currNode.x)
      newDirection = "E";
    else if (nextNode.x < currNode.x)
      newDirection = "W";
    else if (nextNode.y > currNode.y)
      newDirection = "N";
    else if (nextNode.y < currNode.y)
      newDirection = "S";

    if (newDirection == currentDirection)
    {

      stepCount++;
    }
    else
    {

      if (stepCount > 0)
      {
        actions.addAction("Move Forward", stepCount);
      }

      if ((currentDirection == "N" && newDirection == "E") || (currentDirection == "E" && newDirection == "S") || (currentDirection == "S" && newDirection == "W") || (currentDirection == "W" && newDirection == "N"))
      {
        actions.addAction("Turn Right");
      }
      else if ((currentDirection == "N" && newDirection == "W") || (currentDirection == "W" && newDirection == "S") || (currentDirection == "S" && newDirection == "E") || (currentDirection == "E" && newDirection == "N"))
      {
        actions.addAction("Turn Left");
      }
      else
      {
        actions.addAction("U-Turn");
      }

      currentDirection = newDirection;
      stepCount = 1;
    }

    current = current->next;
  }

  if (stepCount > 0)
  {
    actions.addAction("Move Forward", stepCount);
  }

  return actions;
}

int main()
{
  DFS();

  ActionList actions = finalpath(path);

  actions.printActions();

  return 0;
}