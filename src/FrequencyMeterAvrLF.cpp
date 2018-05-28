#include <Arduino.h>

#include <util/atomic.h>


#include <LiquidCrystal.h>
LiquidCrystal lcd(0, 1, 2, 3, 4, 6, 7);  // RS, RW (optional), E, D4-7
#define BUFFER_SIZE 25
char buffer[BUFFER_SIZE];  // buffer for raw of LCD


/*
 *  Accumulate number of pulses of Timer/Counter1' filling frequency in each one
 *  period of input signal for averaging so
 *
 *                periods_accumulator
 *    period = -------------------------
 *              periods_accumulator_cnt
 *
 */
uint32_t periods_accumulator = 0;
uint32_t periods_accumulator_cnt = 0;

double frequency;
double previous_frequency = 0.0;

/*
 *  We use additional "prescaler" (integer variable) to get whether 1-second or
 *  1/3-second updating period. Bigger period is used when input measured frequency
 *  is stable while lesser one is for tuning mode (when input frequency changes rapidly)
 */
#define TIMER0_ADDITIONAL_PRESCALER_NORMAL_MODE 249
#define TIMER0_ADDITIONAL_PRESCALER_TUNING_MODE 83
uint8_t timer0_additional_prescaler = TIMER0_ADDITIONAL_PRESCALER_TUNING_MODE;
uint8_t timer0_additional_prescaler_cnt = 0;
/*
 *  Changing in 1% of current frequency mean that user is tuning right now
 *  so we need to increase refresh rate
 */
#define FREQUENCY_OFFSET 0.01

/*
 *  F_CPU divided by this give us the frequency of TC1 (i.e. filling frequency)
 */
#define FILLING_FREQUENCY_PRESCALER 256.0



int main() {

    ATOMIC_BLOCK(ATOMIC_FORCEON) {

        lcd.begin(16, 2);
    	lcd.clear();
    	lcd.print("starting...");

        /*
         *  Timer0 setup (LCD refreshing)
         */
        TCCR0A |= (1<<WGM01);   // CTC mode
        OCR0A = 250;
        TIMSK0 |= (1<<OCIE0A);  // enable compare interrupt
        TCCR0B |= (1<<CS02);    // prescaler FILLING_FREQUENCY_PRESCALER and start the timer

        /*
         *  Timer1 setup (Input Capture mode, on rising edges)
         */
        TCCR1B |= (1<<ICNC1);   // noise canceler (4-cycles delay)
        TIMSK1 |= (1<<ICIE1);   // input capture interrupt ON
        TCCR1B |= (1<<CS12);    // 256 precsaler and start the timer

    }

    while (1) {}
}



/*
 *  Accumulate number of pulses of Timer/Counter1' filling frequency in each one
 *  period of input signal for averaging so
 *
 *                periods_accumulator
 *    period = -------------------------
 *              periods_accumulator_cnt
 *
 *
 *        __________________________                                     ____
 *       /▉▉  ▉▉  ▉▉  ▉▉  ▉▉  ▉▉  ▉▉\ ▉▉  ▉▉  ▉▉  ▉▉  ▉▉  ▉▉  ▉▉  ▉▉  ▉▉/
 *      / ▉▉  ▉▉  ▉▉  ▉▉  ▉▉  ▉▉  ▉▉ \▉▉  ▉▉  ▉▉  ▉▉  ▉▉  ▉▉  ▉▉  ▉▉  ▉▉
 *   __/__▉▉__▉▉__▉▉__▉▉__▉▉__▉▉__▉▉__▉▉__▉▉__▉▉__▉▉__▉▉__▉▉__▉▉__▉▉__▉▉_____
 *         1   2   3   4   5   6   7   8   9  10  11  12  13  14  15 /16
 *   start on                           \                           /
 *   rising edge                         \                         /
 *                                        -------------------------
 *
 *
 *  On each interrupt fire, content of counting register (TCNT1) is copied into
 *  ICR1. We also need to subtruct the time of ISR entering (in ticks) because
 *  TCNT1 isn't cleared automatically
 */
ISR (TIMER1_CAPT_vect, ISR_NAKED) {

    ATOMIC_BLOCK(ATOMIC_FORCEON) {

        TCNT1 -= ICR1;
        periods_accumulator += ICR1;
        periods_accumulator_cnt++;

    }

    reti();
}



/*
 *  Timer for LCD. Inside it we use additional "prescaler" (integer variable) to
 *  get whether 1-second or 1/3-second updating period. Bigger period is used
 *  when input measured frequency is stable while lesser one is for tuning mode
 *  (when input frequency changes rapidly)
 */
ISR (TIMER0_COMPA_vect) {

    ATOMIC_BLOCK(ATOMIC_FORCEON) {

        // Additional prescaler so we display the measured frequency much less
        // often (also averaging)
        if (++timer0_additional_prescaler_cnt == timer0_additional_prescaler) {
            timer0_additional_prescaler_cnt = 0;

            frequency = (((double)F_CPU)/FILLING_FREQUENCY_PRESCALER) *\
                        (((double)periods_accumulator_cnt)/((double)periods_accumulator));

            // NaN case (no input signal i.e. division by 0)
            if (frequency != frequency)
                frequency = 0.0;

            periods_accumulator = 0;
            periods_accumulator_cnt = 0;

            // If frequency is changed too often we assume that we are in the Tuning
            // mode and switch to more frequent rate of LCD updates
            if ( fabs(frequency-previous_frequency) > (previous_frequency*FREQUENCY_OFFSET) )
                timer0_additional_prescaler = TIMER0_ADDITIONAL_PRESCALER_TUNING_MODE;
            else
                timer0_additional_prescaler = TIMER0_ADDITIONAL_PRESCALER_NORMAL_MODE;
            previous_frequency = frequency;

            lcd.clear();
            sprintf(buffer, "%g Hz", frequency);
            lcd.print(buffer);
        }

    }

}
