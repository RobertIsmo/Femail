from docker.io/debian

run apt-get update
run DEBIAN_FRONTEND=noninteractive apt-get install -y \
	ca-certificates;

copy ca/mail.mailey.femail.crt /usr/local/share/ca-certificates/mail.mailey.femail.crt
copy ca/mail.failey.femail.crt /usr/local/share/ca-certificates/mail.failey.femail.crt
copy ca/mail.mailey.femail.crt /usr/share/ca-certificates/mail.mailey.femail.crt
copy ca/mail.failey.femail.crt /usr/share/ca-certificates/mail.failey.femail.crt

run update-ca-certificates
