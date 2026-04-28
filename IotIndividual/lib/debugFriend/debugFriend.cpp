#include "debugFriend.h"

#include "Arduino.h"
#include "../../include/CommonDefs.h"

void setLEDStatusRED()
{
	digitalWrite(STATUS_BLED, LOW);
	digitalWrite(STATUS_GLED, LOW);

	digitalWrite(STATUS_RLED, HIGH);
}

void setLEDStatusGREEN()
{
	digitalWrite(STATUS_BLED, LOW);
	digitalWrite(STATUS_RLED, LOW);

	digitalWrite(STATUS_GLED, HIGH);
}

void setLEDStatusBLUE()
{
	digitalWrite(STATUS_RLED, LOW);
	digitalWrite(STATUS_GLED, LOW);

	digitalWrite(STATUS_BLED, HIGH);
}

void setLEDStatusMAGENTA()
{
	digitalWrite(STATUS_RLED, HIGH);
	digitalWrite(STATUS_GLED, LOW);

	digitalWrite(STATUS_BLED, HIGH);
}

void setLEDStatusOFF()
{
	digitalWrite(STATUS_RLED, LOW);
	digitalWrite(STATUS_GLED, LOW);
	digitalWrite(STATUS_BLED, LOW);
}

void unrecoverableErrorStatus()
{
	setLEDStatusOFF();

	for(;;){
		digitalWrite(STATUS_RLED, HIGH);
		delay(500);
		digitalWrite(STATUS_RLED, LOW);

		digitalWrite(STATUS_BLED, HIGH);
		delay(500);
		digitalWrite(STATUS_BLED, LOW);

		digitalWrite(STATUS_RLED, HIGH);
		delay(200);
		digitalWrite(STATUS_RLED, LOW);
		delay(100);
		digitalWrite(STATUS_RLED, HIGH);
		delay(200);
		digitalWrite(STATUS_RLED, LOW);

		digitalWrite(STATUS_BLED, HIGH);
		delay(200);
		digitalWrite(STATUS_BLED, LOW);
		delay(100);
		digitalWrite(STATUS_BLED, HIGH);
		delay(200);
		digitalWrite(STATUS_BLED, LOW);
	}
}