#include<iostream>
#include<bits/stdc++.h>

using namespace std;

class Node {
public:
  int x, y;
  Node(int x = 0, int y = 0)
    : x(x), y(y) {}
  bool equals(const Node& other) const {
    return x == other.x && y == other.y;
  }
};

class ListNode {
public:
  Node data;
  ListNode* next;
  ListNode* prev;
  ListNode(Node node)
    : data(node), next(nullptr), prev(nullptr) {}
};

class LinkedList {
public:
  ListNode* head;
  ListNode* tail;
  int elements;

  LinkedList()
    : head(nullptr), tail(nullptr), elements(0) {}

  void addEnd(Node node) {
    ListNode* newNode = new ListNode(node);
    if (tail == nullptr) {
      head = tail = newNode;
    } else {
      tail->next = newNode;
      newNode->prev = tail;
      tail = newNode;
    }
  }

  void addpathEnd(Node node) {
    ListNode* newNode = new ListNode(node);
    if (tail == nullptr) {
      head = tail = newNode;
    } else {
      tail->next = newNode;
      newNode->prev = tail;
      tail = newNode;
    }
    elements++;
  }

  void removeEnd() {
    if (tail != nullptr) {
      ListNode* temp = tail;
      tail = tail->prev;
      if (tail != nullptr) {
        tail->next = nullptr;
      } else {
        head = nullptr;
      }
      delete temp;
    }
  }

  void removepathEnd() {
    if (elements == 1) {
      if (tail != nullptr) {
        ListNode* temp = tail;
        tail = tail->prev;
        if (tail != nullptr) {
          tail->next = nullptr;
        } else {
          head = nullptr;
        }
        delete temp;

        LinkedList path;
        Node initial(0, 0);
        path.addpathEnd(initial);
      }
    } else if (tail != nullptr) {
      ListNode* temp = tail;
      tail = tail->prev;
      if (tail != nullptr) {
        tail->next = nullptr;
      } else {
        head = nullptr;
      }
      delete temp;
    }
    elements--;
  }

  Node getEnd() {
    return (tail != nullptr) ? tail->data : Node(-1, -1);
  }

  bool isEmpty() {
    return tail == nullptr;
  }

  bool contains(Node n) {
    ListNode* current = head;
    while (current != nullptr) {
      if (current->data.equals(n)) {
        return true;
      }
      current = current->next;
    }
    return false;
  }

  void printPath() {
    ListNode* current = head;
    cout << "Current Path: ";
    while (current != nullptr) {
      cout << "(" << current->data.x << ", " << current->data.y << ") ";
      current = current->next;
    }
    cout << endl;
  }
};

string TurnLeft(string dir) {
  if (dir == "N") return "W";
  if (dir == "W") return "S";
  if (dir == "S") return "E";
  return "N";
}

string Uturn(string dir) {
  if (dir == "N") return "S";
  if (dir == "W") return "E";
  if (dir == "S") return "N";
  return "W";
}

string TurnRight(string dir) {
  if (dir == "N") return "E";
  if (dir == "E") return "S";
  if (dir == "S") return "W";
  return "N";
}

Node MoveForward(int& x, int& y, string direction) {
  if (direction == "N") y++;
  else if (direction == "S") y--;
  else if (direction == "E") x++;
  else if (direction == "W") x--;

  return Node(x, y);
}

LinkedList path;         
LinkedList visited;      
string direction = "N";  
double distances[4];

void DFS() {
  int x = 0, y = 0;
  Node start(x, y);
  path.addpathEnd(start);
  visited.addEnd(start);

  while (true) {

    path.printPath();

    cout << "Enter front1 distance: ";
    cin >> distances[0];
    cout << "Enter front2 distance: ";
    cin >> distances[1];
    cout << "Enter left distance: ";
    cin >> distances[2];
    cout << "Enter right distance: ";
    cin >> distances[3];

    if (distances[0] == -1 && distances[1] == -1 && distances[2] == -1 && distances[3] == -1) {
      cout << "Exiting program as per user input." << endl;
      break;
    }

    int front1Distance = distances[0];
    int front2Distance = distances[1];
    int leftDistance = distances[2];
    int rightDistance = distances[3];

    int frontDistance = (front1Distance + front2Distance) / 2;

    Node next_pos(x, y);
    if (direction == "N") next_pos.y++;
    else if (direction == "S") next_pos.y--;
    else if (direction == "E") next_pos.x++;
    else if (direction == "W") next_pos.x--;

    if (frontDistance > 15 && !visited.contains(next_pos))
    {
      MoveForward(x, y, direction);
      path.addpathEnd(next_pos);
      visited.addEnd(next_pos);
      cout<<"Moving forward"<<endl;
    } 

    else if (leftDistance > 15) 
    {
      direction = TurnLeft(direction);
      cout<< "Turning Left"<<endl;
    } 
    
    else if (rightDistance > 15) 
    {
      direction = TurnRight(direction);
      cout << "Turning Right" <<endl;
    } 

    else 
    {
        cout<<"Getting into backtracking"<<endl;
        int flag = 1;
        while (true) {

        if (flag)
        {
            direction = Uturn(direction);
            cout<<"Taking a Uturn"<<endl;
            flag =0;
        }
        
        path.printPath(); 

        cout << "Enter front1 distance: ";
        cin >> distances[0];
        cout << "Enter front2 distance: ";
        cin >> distances[1];
        cout << "Enter left distance: ";
        cin >> distances[2];
        cout << "Enter right distance: ";
        cin >> distances[3];

        if (distances[0] == -1 && distances[1] == -1 && distances[2] == -1 && distances[3] == -1) {
          cout << "Exiting program as per user input." << endl;
          return;
        }

        int front1Distance = distances[0];
        int front2Distance = distances[1];
        int leftDistance = distances[2];
        int rightDistance = distances[3];

        int frontDistance = (front1Distance + front2Distance)/2;


        Node right(x, y);
        if (direction == "S") right.x--;
        else if (direction == "N") right.x++;
        else if (direction == "W") right.y++;
        else right.y--;

        Node left(x, y);
        if (direction == "S") left.x++;
        else if (direction == "N") left.x--;
        else if (direction == "W") left.y--;
        else left.y++;

        Node front(x, y);
        if (direction == "S") front.y--;
        else if (direction == "N") front.y++;
        else if (direction == "W") front.x--;
        else front.x++;


        bool checkF = (frontDistance > 15) && path.contains(front) && (front.y >= 0);
        bool checkL = (leftDistance > 15) && path.contains(left) && (left.y >= 0);
        bool checkR = (rightDistance > 15) && path.contains(right) && (right.y >= 0);

        if(frontDistance > 15 && !visited.contains(front)){
            cout<<"Getting out of backtracking and returning to DFS"<<endl;
            MoveForward(x,y,direction);
            cout<<"Moving forward"<<endl;
            path.addEnd(front);
            visited.addEnd(front);
            break;
        }
        else if(leftDistance > 15 && !visited.contains(left)){
            direction = TurnLeft(direction);
            cout<<"Turning Left"<<endl;
        }

        else if(rightDistance > 15 && !visited.contains(right)){
            direction = TurnRight(direction);
            cout<<"Turning Right"<<endl;
        }


        else if (!(checkF || checkL || checkR)) {
            direction = Uturn(direction);
        }

        else if (checkF) {
            MoveForward(x, y, direction);
            path.removepathEnd();
        }

        else if (checkL) {
            direction = TurnLeft(direction);
            cout<<"Turning Left"<<endl;
        }

        else if (checkR) {
            direction = TurnRight(direction);
            cout<<"Turning Right"<<endl;
        }

      }
    }
  }
}


class ActionNode {
public:
  string action; 
  int steps;     
  ActionNode* next;

  ActionNode(string action, int steps = 0)
    : action(action), steps(steps), next(nullptr) {}
};

class ActionList {
public:
  ActionNode* head;
  ActionNode* tail;

  ActionList() : head(nullptr), tail(nullptr) {}

  void addAction(string action, int steps = 0) {
    ActionNode* newNode = new ActionNode(action, steps);
    if (tail == nullptr) {
      head = tail = newNode;
    } else {
      tail->next = newNode;
      tail = newNode;
    }
  }

  void printActions() {
    ActionNode* current = head;
    cout << "Final route: " << endl;
    while (current != nullptr) {
      if (current->action == "Move Forward") {
        cout << current->action << " " << current->steps << " steps" << endl;
      } else {
        cout << current->action << endl;
      }
      current = current->next;
    }
  }
};

ActionList finalpath(LinkedList& path) {
  ActionList actions;
  
  ListNode* current = path.head;
  string currentDirection = "N";
  int stepCount = 0;

  while (current->next != nullptr) {
    Node currNode = current->data;
    Node nextNode = current->next->data;

    string newDirection;
    if (nextNode.x > currNode.x) newDirection = "E";
    else if (nextNode.x < currNode.x) newDirection = "W";
    else if (nextNode.y > currNode.y) newDirection = "N";
    else if (nextNode.y < currNode.y) newDirection = "S";

    if (newDirection == currentDirection) {

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
      else if ((currentDirection == "N" && newDirection == "W") || (currentDirection == "W" && newDirection == "S") ||(currentDirection == "S" && newDirection == "E") || (currentDirection == "E" && newDirection == "N")) 
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

  if (stepCount > 0) {
    actions.addAction("Move Forward", stepCount);
  }

  return actions;
}


int main() {
  DFS(); 

  ActionList actions = finalpath(path);

  actions.printActions();

  return 0;
}

