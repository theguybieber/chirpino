# Hummingbird

#### A small, simple API to get your IoT devices chirping

Version 1.0 May 2015
**[Chirp](http://chirp.io)**

This short document explains how programs may interact with hummingbird.chirp.io to create and query chirp codes. It is written for developers who wish to write their own programs using this API. A ready-written library **ChirpinoLink** suitable for certain Atmel-based Arduinos is also being made available.

The **Hummingbird API** has been designed to be as simple as possible to provide an easy way to get your Arduinos and other small devices chirping. Your device communicates with the `hummingbird.chirp.io` server using short exchanges of JSON. Everything is sent without any complex encryption or authentication but you do need to obtain and use a personal API key. You can create new chirps for text messages and for web links. This content is stored at `chirp.io` and you receive a chirp code to play that directs to this content. You can also query chirp codes to obtain content associated with them.

The **ChirpinoLink** library has been created to provide the functionality required to interact with the hummingbird server. A separate library, **ChirpinoSing** lets you synthesize chirp sounds directly from your Arduino pins. An example program, **Chirpino**, has been created as a full demonstration of the use of  **ChirpinoLink**, **ChirpinoSing** and the **Hummingbird API**. With a full serial monitor command interface, it lets you create, store and manage lists of chirps and play them on demand or at timed intervals.

You don't need to use **ChirpinoLink** to interact with the server, and at the moment it only supports certain Arduinos using the Ethernet Shield, or the Spark Core. However if you are writing your own implementation you may wish to refer to it as example code. 

#### Your API key

All requests to hummingbird must be sent with a 22-character base-64 api key. This is personal to you and your devices; don't share it with others as any abuse of the key such as making an unreasonable number of requests is traceable directly to you.

You supply in a custom HTTP request header

	X-chirp-hummingbird-key: 0123456789abcdefghijkl

Requests not including a valid key are rejected.

To obtain your own key visit [Chirp](http://chirp.io/hello-developers/) where you'll be asked to fill in a very short form.


#### Time

All responses include a field called `now` whose value is the approximate current time in UTC in the form `hh:mm:ss`. This is designed to give devices without real-time clocks a means to provide time-based services.


#### Codes

Chirp codes are 18 characters long and may contain digits, and letters in the range a to v. These characters correspond to tones that are played when the code is chirped. Two additional tones precede each chirp.


#### JSON

As with all JSON, the order of named fields is arbitrary so may differ from the examples below. Most strings should just contain ASCII printable characters, although escape sequences such as \t and \n may be used within the body of text messages.


#### Error responses

If the server is unable to process your request you will receive an error status code in the HTTP response and no JSON payload.


#### Create a chirp for a URL

Your URL value should begin with `http://`. The title is any short piece of text, although commonly the domain section of the url is used. You receive a freshly-minted chirp code. Eg


POST to `hummingbird.chirp.io:1254/chirp`

	{
		"mimetype": "text/url",
		"url": "http://example.com/path/whatever",
		"title": "example.com"
	}

response:

	{
		"now": "12:34:56",
		"code": "0123456789abcdefgh"
	}


#### Create a chirp for a text message

This is similar to the above, but the text is supplied in a field called `body` and the mimetype is given as `text/plain`. As above, the title may be any short piece of text although here it is conventionally the beginning of the message. Eg

POST to `hummingbird.chirp.io:1254/chirp`

	{
		"mimetype": "text/plain",
		"body": "A message that you wish to chirp",
		"title": "A message that you..."
	}

response:

	{
		"now": "12:34:56",
		"code": "0123456789abcdefgh"
	}


#### Fetch chirp details

Supply a valid chirp code at the end of the url to retrieve its details from the server. Eg

GET from `hummingbird.chirp.io:1254/chirp/f3jhi0kcfgola4ujcj`

response:

	{
		"now": "11:02:19"
		"mimetype": "text/plain",
		"body": "hello",
		"title": "Message",
		"created_at": "2015-01-27T11:03:38Z",
	}

You can supply any chirp code, whether or not it was created using this API. In order to avoid overwhelming the limited buffers of small devices only a subset of information stored about general chirps is returned here.

The times supplied for the `now` and `created_at` fields are based on different clocks so may differ; as here where the chirp claims to have been created in the future.


#### A simple time request

This can be used whenever a device needs to know the time. As with all responses the format is "hh:mm:ss" in approximate UTC. Eg

GET from `hummingbird.chirp.io:1254/now`

response:

	{
		"now": "12:34:56"
	}
