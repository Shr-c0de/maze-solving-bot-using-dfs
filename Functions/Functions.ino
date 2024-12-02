


class Sensor {
  int sensors[] = { 4, 5, 6, 13 };

public:
  Sensor() {
    for (int i = 0; i < 4; i++) pinMode(sensors[i], OUTPUT);
  }
};


void setup() {
}

void loop(){
  // put your main code here, to run repeatedly:

}[4, 5, 6, 13]