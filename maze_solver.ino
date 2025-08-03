// Smart Maze Solver: Obstacle Distance-Based Speed + Safe Turning

// Motor pins
const int enA = 9, in1 = 8, in2 = 7;
const int enB = 3, in3 = 5, in4 = 4;

// Ultrasonic pins
const int trigFront = 11, echoFront = 12;
const int trigLeft  = 6, echoLeft  = 10;
const int trigRight = A0, echoRight = A1;

// Distance thresholds (in cm)
const int maxDistance = 100;
const int stopThreshold = 10;
const int turningThreshold = 15;
const int reactionThreshold = 30;

// Speed limits
const int minSpeed = 80;
const int maxSpeed = 150;

// Turn timings (tune these!)
const int turnDelay = 700;
const int uTurnDelay = 1300;

unsigned long lastDisplayTime = 0;
const long displayInterval = 250;

long dFront = 0, dLeft = 0, dRight = 0;

void setup() {
  Serial.begin(9600);

  pinMode(enA, OUTPUT); pinMode(enB, OUTPUT);
  pinMode(in1, OUTPUT); pinMode(in2, OUTPUT);
  pinMode(in3, OUTPUT); pinMode(in4, OUTPUT);

  pinMode(trigFront, OUTPUT); pinMode(echoFront, INPUT);
  pinMode(trigLeft, OUTPUT);  pinMode(echoLeft, INPUT);
  pinMode(trigRight, OUTPUT); pinMode(echoRight, INPUT);

  stopMotors();
  Serial.println("SMART MAZE ROBOT: Distance-based Speed + Smart Navigation");
  delay(2000);
}

long getDistance(int trig, int echo) {
  digitalWrite(trig, LOW); delayMicroseconds(2);
  digitalWrite(trig, HIGH); delayMicroseconds(10);
  digitalWrite(trig, LOW);
  long duration = pulseIn(echo, HIGH, 10000);
  if (duration == 0) return maxDistance;
  return duration * 0.0343 / 2;
}

void updateDistances() {
  dFront = getDistance(trigFront, echoFront);
  dLeft  = getDistance(trigLeft, echoLeft);
  dRight = getDistance(trigRight, echoRight);
}

int getSpeedFromDistance(long distance) {
  if (distance >= reactionThreshold) return maxSpeed;
  if (distance <= stopThreshold) return 0;
  return map(distance, stopThreshold, reactionThreshold, minSpeed, maxSpeed);
}

void displayStatus(String action, int speed) {
  if (millis() - lastDisplayTime >= displayInterval) {
    lastDisplayTime = millis();
    Serial.print("Front: "); Serial.print(dFront); Serial.print(" cm | ");
    Serial.print("Left: "); Serial.print(dLeft); Serial.print(" cm | ");
    Serial.print("Right: "); Serial.print(dRight); Serial.print(" cm | ");
    Serial.print("Speed: "); Serial.print(speed); Serial.print(" | ");
    Serial.println("Action: " + action);
  }
}

void loop() {
  updateDistances();

  if (dFront > reactionThreshold) {
    moveForward(maxSpeed);
    displayStatus("Moving Forward", maxSpeed);
  }
  else if (dFront > turningThreshold && dFront <= reactionThreshold) {
    int slowSpeed = getSpeedFromDistance(dFront);
    moveForward(slowSpeed);
    displayStatus("Slowing Down", slowSpeed);
  }
  else {
    stopMotors();
    delay(100);

    updateDistances();

    if (dLeft > turningThreshold && dRight > turningThreshold) {
      if (dLeft > dRight) {
        turnWithCheck("LEFT");
      } else {
        turnWithCheck("RIGHT");
      }
    }
    else if (dLeft > turningThreshold) {
      turnWithCheck("LEFT");
    }
    else if (dRight > turningThreshold) {
      turnWithCheck("RIGHT");
    }
    else {
      performUTurn();  // <-- Updated logic inside this function
    }
  }

  delay(100);
}

// ------------------ Movement ------------------

void moveForward(int speed) {
  digitalWrite(in1, HIGH); digitalWrite(in2, LOW);
  digitalWrite(in3, HIGH); digitalWrite(in4, LOW);
  analogWrite(enA, speed); analogWrite(enB, speed);
}

void stopMotors() {
  digitalWrite(in1, LOW); digitalWrite(in2, LOW);
  digitalWrite(in3, LOW); digitalWrite(in4, LOW);
}

void turnRight() {
  digitalWrite(in1, LOW); digitalWrite(in2, HIGH);
  digitalWrite(in3, HIGH); digitalWrite(in4, LOW);
  analogWrite(enA, minSpeed); analogWrite(enB, minSpeed);
  delay(turnDelay);
  stopMotors();
}

void turnLeft() {
  digitalWrite(in1, HIGH); digitalWrite(in2, LOW);
  digitalWrite(in3, LOW); digitalWrite(in4, HIGH);
  analogWrite(enA, minSpeed); analogWrite(enB, minSpeed);
  delay(turnDelay);
  stopMotors();
}

// ------------------ Enhanced U-Turn Logic ------------------

void performUTurn() {
  // Perform U-turn
  digitalWrite(in1, HIGH); digitalWrite(in2, LOW);
  digitalWrite(in3, LOW); digitalWrite(in4, HIGH);
  analogWrite(enA, maxSpeed); analogWrite(enB, maxSpeed);
  delay(uTurnDelay);
  stopMotors();
  displayStatus("U-Turn (Dead End)", minSpeed);

  delay(200);
  updateDistances();

  if (dFront > turningThreshold && dLeft > turningThreshold && dRight > turningThreshold) {
    turnWithCheck("LEFT"); // Explore new path
  }
  else if (dLeft > turningThreshold) {
    turnWithCheck("LEFT");
  }
  else if (dRight > turningThreshold) {
    turnWithCheck("RIGHT");
  }
  else if (dFront > turningThreshold) {
    moveForward(getSpeedFromDistance(dFront));
    displayStatus("Moving Forward after U-Turn", getSpeedFromDistance(dFront));
  }
  else {
    displayStatus("Stuck after U-Turn, waiting...", 0);
  }
}

// ------------------ Smart Turn Check ------------------

void turnWithCheck(String direction) {
  if (direction == "LEFT") {
    turnLeft();
  } else {
    turnRight();
  }

  delay(200); // Small pause before checking again
  updateDistances();

  if (dFront < stopThreshold) {
    performUTurn();
    displayStatus("U-Turn after Failed Turn", minSpeed);
  } else {
    displayStatus("Turned " + direction + " and Found Path", minSpeed);
  }
}
