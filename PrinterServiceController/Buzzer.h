#pragma once
class Buzzer
{
private:
	static void setup();
	static inline bool isSetup = false;
public:
	static void singleBuzz();
	static void doubleBuzz();
};

