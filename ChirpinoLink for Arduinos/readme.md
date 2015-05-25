#ChirpinoLink

###for Arduino with Ethernet Shield
Version 1.0 May 2015

[chirp.io](http://chirp.io)

###Overview
ChirpinoLink is a library enabling Arduinos to interact with the hummingbird.chirp.io server to create new chirps for pieces of text and for URLs, and to query chirp codes for the information associated with them.

The highly simplified JSON API that is used to let your device communicate with Chirp is outlined in the nearby document **hummingbird API 1.0.md**.

Using ChirpinoLink requires that you obtain an API key from Chirp. You can sign up for one **[here](http://chirp.io/hello-developers/)**.

One example program is provided at present, and more will follow very shortly, together with more extensive documentation. The Chirpino program for Arduino Mega2560 with ethernet card (also available in this chirpino repository) makes extensive use of ChirpinoLink together with the audio synthesis library ChirpinoSing. You can regard the Chirpino program as a very large example program for both ChirpinoLink and ChirpinoSing.