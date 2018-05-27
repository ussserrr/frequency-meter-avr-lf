#include <Arduino.h>
#include <LiquidCrystal.h>

#include <util/atomic.h>


LiquidCrystal lcd(0, 1, 2, 3, 4, 6, 7);  // RS, RW (optional), E, D4-7
#define BUFFER_SIZE 25
char bufferA[BUFFER_SIZE];  // 1st raw of LCD
// char bufferB[BUFFER_SIZE];  // 2nd raw of LCD
// #define LCD_TIMER_PRESCALER 249
uint8_t lcd_timer_prescaler_cnt = 0;

double frequency;
uint32_t period_accumulator = 0;
uint32_t period_accumulator_cnt = 0;
// bool is_first_measurement = true;

double previous_frequency = 0.0;
uint8_t timer2_additional_prescaler = 83;
// uint8_t timer2_additional_prescaler_cnt = 0;
#define FREQUENCY_OFFSET 0.01



int main() {
    cli();

    lcd.begin(16, 2);
	lcd.clear();
	lcd.print("starting...");

    /* Timer0 setup (LCD) */
    TCCR0A |= (1<<WGM01);  // CTC mode
    OCR0A = 250;
    TCCR0B |= (1<<CS02);  // prescaler - 256
    TIMSK0 |= (1<<OCIE0A);

    /* Timer1 setup */
    // // Input capture mode: rising edge
    // TCCR1B |= (1<<ICES1);
    // Noise canceler (4-cycles delay)
    TCCR1B |= (1<<ICNC1);
    // Input capture interrupt ON
    TIMSK1 |= (1<<ICIE1);
    // 256 precsaler and start timer
    TCCR1B |= (1<<CS12);

    // DDRC |= (1<<PC2);

    sei();

    while (1) {}
}


ISR (TIMER1_CAPT_vect, ISR_NAKED) {
    // cli();
    ATOMIC_BLOCK(ATOMIC_FORCEON) {

    // if (is_first_measurement) {
    //     is_first_measurement = false;
    //     reti();
    // }

    TCNT1 -= ICR1;
    period_accumulator += ICR1;
    period_accumulator_cnt++;
    }

    // sei();
    reti();
}


ISR (TIMER0_COMPA_vect) {
    // cli();
    ATOMIC_BLOCK(ATOMIC_FORCEON) {

    if (++lcd_timer_prescaler_cnt == timer2_additional_prescaler) {
        // PORTC ^= (1<<PC2);
        lcd_timer_prescaler_cnt = 0;

        frequency = (16000000.0f/256.0f)*((double)period_accumulator_cnt/period_accumulator);
        if (frequency != frequency)
            frequency = 0.0;

        period_accumulator = 0;
        period_accumulator_cnt = 0;
        // is_first_measurement = true;

        if ( fabs(frequency-previous_frequency) > (previous_frequency*FREQUENCY_OFFSET) ) {
            timer2_additional_prescaler = 83;
        }
        else {
            timer2_additional_prescaler = 249;
        }
        previous_frequency = frequency;

        lcd.clear();
        sprintf(bufferA, "%g Hz", frequency);///1000.0f);
        lcd.print(bufferA);
    }

    // sei();
    }

    // reti();
}
