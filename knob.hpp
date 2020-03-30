
class Knob {
  const float value_min;
  const float value_max;
  const uint16_t pin;
  const int samples;
public:
  Knob(const uint16_t pin, const float value_min=0, const float value_max=1024.0, const int samples=25) :
    value_min(value_min),
    value_max(value_max),
    samples(samples),
    pin(pin) {};

  float read() {
    float average = 0;
    int count = samples;

    while (count--)
	   average += analogRead(pin);

    average /= samples;

    return value_min + ((value_max - value_min) * average) / 1023.0;
  };
};
