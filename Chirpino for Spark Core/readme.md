# Chirpino
###for Spark Core

Version 1.0 May 2015
**[Chirp](chirp.io)**

## Overview



**Chirpino** is a program for Arduinos and other microcontrollers to let you create, play and store lists of chirps; this document describes the version for Spark Core. **Chirpino** lets you store playlists of chirp codes and create chirps for text messages and URLs so they can be picked up by anyone with a phone or other device running the free **Chirp** app. You can use its functionality as written or you can use it as an example to help you create your own programs exploring the world of chirps.

**Chirpino** uses the Spark Core's external flash to store chirp playlists even when the power is off. By default, each of the four playlists has room for up to ten chirp codes. When new chirp codes are written to a full playlist they overwrite the oldest ones so in each playlist only the most recently added ten are stored.

Playlists hold settings such as play order, audio preferences, and timing information for sequenced or timed play. Each can also be given a URL to a web-based script holding commands and/or chirp codes to add. 

This document includes the following sections:

* **Getting Started** - required hardware and software, obtaining your API key, and how to run the program
* **Usage** - the command line interface to **Chirpino**; a large number of very simple commands
* **Scripting** - making lists of chirps and/or command sequences available on the web to update your playlists

In addition, two other documents are available:

* **Hummingbird API** - a guide to the available JSON exchanges with the hummingbird.chirp.io server
* **ChirpinoSing.pdf** - full documentation on the **ChirpinoSing** sound output library and its example programs. **ChirpinoSing** was written as an Arduino library, a port of which is bundled here together with the main **Chirpino for Spark Core** source as the files **Beak**, **PortamentoBeak** and **Synth**.


## Getting started

### Required hardware

This version of **Chirpino** is written for the Spark Core.

You'll need a small speaker or an earphone for sound output, or you can open up a 3.5mm audio jack and connect to a larger audio device. Use **pin 10** (A0) and **ground** (GND). Although an amplifier and filtering circuitry might improve the power and quality of the sound output, we have had good results from just connecting either an earphone or an old radio AUX-in via an audio jack wired directly to the pins. In both these cases our sound output is probably technically outside the range expected by the device, so connect at your own risk. The **ChirpinoSing** library documentation describes the sound output and the synthesis proces in more detail.

You can control the Spark Core by sending commands from **Chirpino**'s extensive command set using serial-over-usb when it's connected by usb to a computer. Interaction with the hummingbird.chirp.io server to create or fetch information on chirps, or to obtain the current time, requires a wi fi connection and a personal API key. You can also use use Spark cloud commands to control Chirpino when it's on wi fi.

### Software dependencies

**Chirpino** for Spark Core contains all the source files that you need.

### Your API key

To use the **Hummingbird API** you need to obtain a personal API key from **[here](chirp.io/hello-developers/)** and paste it into **PlayerLink.cpp**. This is a 22-character base-64 string that is used as an access token. All interaction with the **hummingbird** server requires this key to be supplied; don't share your key with others as any abuse - such as making an unreasonable number of requests - is traceable back to you.

### Running the program

Ensure you've pasted your API key into **PlayerLink.h** and that your Spark Core is connected and selected, then compile and upload the program. Once the upload is complete open up a serial monitor (the Arduino one works well; set the options at the bottom of the window to 115200 baud and Newlines). You should soon see some messages. Now type a plus sign followed by a few words into the text input box (eg `+hello world`) at the top of the monitor and hit return. If all goes well you should see more messages and hear the new chirp you have just created played out loud through your connected audio device.

### Troubleshooting

If the network initialises but you're receiving errors from the server then check your API key has been entered correctly. Uncommenting the `#define DEBUG_NETWORK` line at the top of **Network.cpp** turns on the printing of full HTTP requests and responses to and from the server. Note that even if **Chirpino** fails to initialise the network on startup it tries again whenever a network request is issued. Typing ^ and return into the serial monitor issues the simple fetch time request and is a good way to check your connection.

## Usage

Interaction with **Chirpino** is by text commands over a serial monitor (set it to 115200 baud, newlines).

Commands are generally one or two punctuation characters. Parameters immediately follow the lead character without a space. Even though many commands are just single characters you always need to press return or hit the send button to issue the command.


### Add new chirps

Start your line with a plus sign then type a short message or URL to make a new chirp referring to this message or URL

Egs

	+This is my first new chirp

	+chirp.io/faq/

	+Chirps are a cute way to pass links to friends

These commands require wifi as the message or URL is sent to `hummingbird.chirp.io` where it is stored in the big database of chirps and a code that references it is returned. This code is then played audibly through your speaker or whatever audio device you have connected. Any listening devices that are in range to hear this chirp, such as phones running the free chirp app, will retrieve and display your message or URL. The new code is added to your current playlist and stored on your device.

URLs and text are handled slightly differently from each other. URLs are distinguished from text messages in the `isAnURL` function in **Commands.cpp**; the string is treated as a URL if it has a minimum length, no spaces and includes at least one dot.

### Add existing chirp codes

If you have an 18-character-long chirp code then simply type or paste it in to add it to your current playlist, eg

	ntdb982ilj6etj6e3l

Codes added like this don't play automatically; use the `!` command (below) to play your newly added code. You don't need a working wifi connection to add codes.

#### Playing chirps

New chirps are played when they are created. Chirpino keeps track of the current playlist and the current chirp. When you add a chirp it becomes the current one.

`!` play the current chirp

`]` play the next chirp within the playlist (this is a more recent one, except where the sequence wraps round to the oldest again)

`[` play the previous chirp (one added earlier, except where the sequence wraps around to the newest one)

`)` play the next chirp but chain the playlists together (so the first chirp of the next playlist is played once the current playlist is completed)

`(` play the previous chirp, but chain the playlists (in reverse order)

`,` play the next chirp in autoplay order (see below)

`!number` select & play the chirp at this index position. Eg

	!4

plays the chirp at the fifth position (counting from 0) in the current playlist.

`!code` plays the given 18 character chirp code (containing only digits and/or letters 'a' to 'v') **without** storing it. Eg

	!oscu7ih85kdq6hl3il

#### Fetching information about a chirp

`?` requests information about the current chirp code from the server and prints it

#### Playlists

You can select the current playlist by its index `0` to `3` or you can step forward or back through them. Most commands apply only to the selected playlist.

`@number` selects the playlist with the given index. Eg

	@0
	
selects the first playlist

`}` selects the next playlist (wrapping back to 0 where appropriate)

`{` selects the preceding playlist (wrapping round to 3 where appropriate)


#### Clearing

`~` clears the current playlist's chirp codes

`~number` clears a combination of items in the current playlist: add `1` to clear chirp codes, `2` to clear settings, `4` to clear script url. Eg

	~6
	
(2 + 4) leaves the chirp codes intact but clears the settings (including sound and timer preferences) and the script url.

`~#` clears the current playlist's script url (same as `~4`)

`~!` clears everything in **ALL** playlists, effectively resetting the playlist storage


#### Audio

`:number` set the volume to the given value (0 to 255). Very low values are not only quieter but result in lower quality sound output.

`:` set the default volume (the maximum, 255)

`:P` use Portamento chirps (these slide between tones)

`:N` use plain (Non-portamento) chirps (these change tones abruptly)

`:M` Mute or unMute sounds (toggle)

Whereas `L` and `P` (and volume) affect the stored settings for the current playlist, `M` simply turns ALL sound output on or off and does not store the setting. The letters here aren't case-sensitive ('`p`, `n` and `m` are fine).

You can combine chirp style and volume. Enter the letter before the volume value. Eg

	:p128

sets portamento at half volume (numerically, if not in loudness).


#### Autoplay

A whole playlist (or the next few chirps - see `|`) can be played instead of just a single chirp. This autoplaying can be triggered by a `*` command or can be set to occur at particular times.

##### Intervals

`.number` sets the time in seconds between the start of successive chirps during autoplay. Egs

	.20

plays the chirps at 20 second intervals.

	.

sets the default interval of 10 seconds.

##### Orders

Choose from a number of autoplay **orders** (stored in the playlist's settings):

`.F` forwards (the default); start with the oldest and play each in turn until the most recently added chirp

`.B` backwards; start with the most recent and play each in turn until the oldest

`.S` shuffle; play each chirp once in a random order

`.R` random; play chirps entirely at random perhaps repeating some & omitting others

Setting the autoplay order moves the play position to be the first as appropriate for the sequence (eg B sets it to be the last chirp in the playlist, F picks the first chirp). These letters aren't case-sensitive.

##### Limits

`|number` (pipe) limits the number of chirps played during autoplay to number, eg

	|2
	
results in only two chirps being played each autoplay.

`|` resets to the default of playing the whole playlist

##### Immediate autoplay

`*`	Autoplays the current playlist immediately

Hitting return or issuing any new command stops this immediate autoplay. Immediate autoplay does not trigger automatic repeats.

##### Timed autoplay

To start autoplay at a particular time use `*HH:MM` where HH and MM represent the hour and minute respectively, each typed as two digits. Egs

	*09:00

to autoplay at 9am, or

	*15:45	

to autoplay at 3:45pm.

In contrast to immediate autoplay, hitting return or issuing commands does not affect timed autoplay. Use mute to stop sound output or `**` to turn off autoplay for the current playlist. Eg

`**` turns timed autoplay off and clears all autoplay settings.

You can also set chirps to autoplay at fixed intervals after the initial time. Specify this as a time interval one space after the first one (and in the same format) then after another space give the total number of autoplays (including the first). Eg

	*11:00 00:10 4

autoplays the playlist at 11am, then again at 11:10, 11:20, and finally 11:30.

`*!` adjusts a preset start time (with possible repeats) to start right now


#### Times

Times are obtained from the chirp server on each network request then millis() is used to track the time elapsed. Server time is (approximately) in UTC. To change them to times for your own locality set an appropriate value for utcOffsetSeconds at the top of Times.cpp or use the `^+` or `^-` commands below.

`^` sends the server a simple time request; it can be useful to test the network as well as to update the time

`^HH:MM:SS` allows you to specify a time in UTC if the network is unavailable. Eg

	^15:30:00
	
sets UTC time to 3:30pm.

`^+seconds` adjusts the time offset from UTC forward by the given number of seconds and stores the new offset, eg

	^+3600

moves the time offset ahead one hour.

`^-seconds` as `^+` but adjusts the time back by this number of seconds
	

#### Scripts

Scripts are text files (or live-generated resources) available on the web (at urls under 50 characters long) that contain lines of commands.

`#url` associates the current playlist with a script at this web address (and stores it with the playlist's settings), eg

	#mysite.example.com/scripts/1.txt
	
sets the playlist's script to refer to the given resource.

`#` fetches & runs the script for the current playlist

`~#` clears the current playlist's script url

`#?` prints the current playlist's settings and chirp codes in a format suitable for using as a script

`#number` sets the playlist's update behaviour, where `#0` prevents automatic updates, `#1` signals to run the script at startup, `#2` signals to run the script just before each autoplay (except immediate autoplays), `#3` implies do both `#1` and `#2`

`/url` runs a script at the given url (without storing the url or associating it with any playlist). Eg

	/mysite.example.com/scripts/meta.txt

`/` runs the scripts associated with ALL playlists

'%' may be used to add comments to scripts; the line is ignored, eg

	%Update chirp list for Wednesday's demo

Since all actual commands (other than chirp codes) begin with punctuation you may prefer just writing your comment text directly; when it isn't understood the command parser will respond `uh?` and ignore the text.

##### Usage

Always start the url with the domain, NOT with the protocol (`http://`), as the parser assumes that the first slash character divides the host domain from the path. Use fewer than 50 characters in the url.

When a script is run the commands it contains are executed as if they were typed at the serial monitor, with the constraints that they can't initiate any other web requests (eg create new chirps or lookup chirp info) since this would overwrite the buffer containing the script, and that no audio plays during script execution (since this reuses the same buffer). The `/` command (on its own, without parameters) and `#` are both fine as these merely request scripts to be played when possible.

Scripts together with their HTTP headers supplied by the server must fit entirely within the send & receive buffer (which is 1000 bytes by default), so you are best limiting them to around 500 characters.

Playlist scripts run in the context of their playlist being the current one; the actual current playlist is restored after the script is run. Scripts run from explicit URLs are run within the context of the existing current playlist, and they may leave this changed.

Scripts may be as simple as a text file containing a list of chirp codes, one per line. The chirp codes are added to the playlist; if there are less than ten and you wish to replace the previous playlist you should issue a clear codes command `~` at the top of the script. Eg

	~
	fdhsjkfhdsfhjdkfhsjk
	fdhsjkfhdsfhjdkfhsjk

A pattern that we have found useful is for one script invoked explicitly by `/url` to set up all playlist scripts & settings, then each individual script be used to update the playlist codes. Eg, the setup script might look like

	@0
	#example.com/path/0.txt
	:P255

	@1
	#bitrivers.com/chirp/dc/1.txt
	quieter non-portamento
	:N120

	@2
	#example.com/path/0.txt
	:P255

	@3
	#example.com/path/0.txt
	:P255

	leave script with playlist 0 as the current one
	@0

	now run all playlist scripts
	/

Note that the parser fails to understand the lines in English and so ignores them as if they were explicit comments.

#### Triggers

Triggers allow you to associate an event with a short script. Chirpino comes with two pins set up as 'buttons'; connect the pin to ground briefly to 'press' the 'button'. A third trigger is an inactivity trigger; if you set a non-zero time in seconds it will trigger whenever no chirp has played for this length of time.

The button tagged `l` (for left) is mapped to the chained previous command `(`. Button `r` is mapped to chained next `)`. The inactivity trigger, tagged `z` is also mapped to `)` so will play the next chirp whenever the unit has been ignored (don't worry: it's off by default).

`=tag` lets you fire triggers from scripts or the command line. Eg

	=r
	
is equivalent to 'pressing' the right button.

`-seconds` lets you set the inactivity timer; use 0 to turn it off. Eg

	-120
	
will cause another chirp to be played every two minutes.

Triggers are most useful when you add your own. Each needs a tag and a script (each of which could, as above, be simply a single character). They have an update method that's called every time around the main loop (so very frequently), and an action method that can be called by update when the conditions are right. You could check sensors, track the time, perform calculations or whatever you like really. Take a look in Triggers.cpp; they're pretty simple.

#### Spark only

Chirpino on the Spark Core exposes a function called `do` to the web using Spark's cloud command system. Any string you supply here will be interpreted as a command, allowing you to control Chirpino from your specially-constructed web page.
