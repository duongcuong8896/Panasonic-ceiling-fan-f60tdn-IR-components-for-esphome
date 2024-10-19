## Coppy folder "f60tdn" to folder "my_components" esphome

```C++
remote_receiver:
  id: rcvr
  dump: raw
  pin:
    number: 5
    inverted: true
    mode:
      input: true
      pullup: False

remote_transmitter:
  id: transmitter1
  pin: 14
  carrier_duty_percent: 50%

fan:
  - platform: f60tdn
    name: "Fan test"
    receiver_id: rcvr
```
   
