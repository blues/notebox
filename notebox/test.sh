set -x
notehub '{"product":"com.blues.notebox","device":"dev:ed153172025c","req":"hub.device.signal","body":{"class":"log","message":"howdy partner"}}'
notehub '{"product":"com.blues.notebox","device":"dev:ed153172025c","req":"hub.device.signal","body":{"id":123,"text":"hi there"}}'
# reply {"id":123,"text":"hi back"}
notehub '{"product":"com.blues.notebox","device":"dev:ed153172025c","req":"hub.device.signal","seconds":30}'
