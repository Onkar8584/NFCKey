#include <atmel_start.h>
#include "MyMain.h"
int main(void)
{
	/* Initializes MCU, drivers and middleware */
	atmel_start_init();
	MyMain_main();
	while (1) {
	}
}
