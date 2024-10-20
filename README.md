How to use ESPHome External Components [phome.io/components/external_components.html](https://esphome.io/components/external_components.html)

Coppy folder "f60tdn" to folder "my_components" of ESPHome and add code:
```C++
fan:
  - platform: f60tdn
    name: "Fan_name"
    has_oscillating: true #(Optional, 1/f Yuragi Mode, Default is true)
    receiver_id: remote_receiver_id #(Optional, The id of the remote_receiver)
```
Example:
```C++
external_components:
  # use all components from a local folder
  - source:
      type: local
      path: my_components

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
   
