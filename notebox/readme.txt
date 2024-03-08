In order to set it up in a notecarrier-f with a swan, route AUX_TX to F_A5.

Both USB and F_RX/F_TX are active for serial I/O

The Notebox concept is simple and threefold:

1. Send a JSON message unidirectionally from the cloud to a device
	a) In the cloud, send your own json to a device
	notehub '{"product":"com.blues.notebox","device":"dev:ed153172025c","req":"hub.device.signal","body":{"yourkey":"yourvalue"}}'
	b) On the USB or UART, you receive it
	{"yourfield":"request"}

2. Send a message unidirectionally from the device to the cloud
	a) On the USB or UART, you send the JSON
	{"yourkey":"yourvalue"}
	d) In the cloud, you see a web request that is sent to an alias 'incoming' whose body field is:
	{"yourkey":"yourvalue"}

3. Do a request/response into a device from the cloud
	a) In the cloud, send your own json request to a device with a timeout, including an ID and including "seconds" timeout
	notehub '{"product":"com.blues.notebox","device":"dev:ed153172025c","req":"hub.device.signal","body":{"id":123,"yourkey":"request"},"seconds":15}'
	b) On the USB or UART, you receive it
	{"id":123,"yourfield":"request"}
	c) On the USB or UART, you send the response
	{"id":123,"yourkey":"response"}
	d) In the cloud, the request issued in (a) comes back with a list of all pending responses, including this one.  You identify it by ID.
    {
        "id": 123,
        "yourkey": "response"
    }
	]

4. Send an unstructured unidirectional newline-delimited text line to the box from the cloud, using the hardwired "class":"log" json
	a) In the cloud, send a message to the 
	notehub '{"product":"com.blues.notebox","device":"dev:ed153172025c","req":"hub.device.signal","body":{"class":"log","message":"howdy partner"}}'
	b) on the USB or UART, you receive
    howdy partner\n

5. Send an unstructured unidirectional newline-delimited text line to the cloud from the box, using the hardwired "class":"log" json
	a) on the USB or UART, you send
    howdy partner\n
	b) In the cloud, you see a web request that is sent to an alias 'incoming' whose body field is:
	{"class":"log,"message:"howdy partner"}
