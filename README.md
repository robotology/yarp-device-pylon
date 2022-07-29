# 1. Hw

## 1.1. Camera model
daA4200-30mci

https://docs.baslerweb.com/embedded-vision/daa4200-30mci

## 1.2. Jumpers

For FW flash:

![jumper](img/flash-jumper.jpg)

For normal use remove it

# 2. SW

## 2.1. robotology repo
On Nvidia board:

```
git clone https://github.com/robotology/yarp-device-basler
```

:warning:_Troubleshooting_
If you haven't yet configured the internet access see below.

## 2.2. Info
https://www.baslerweb.com/en/downloads/software-downloads/#type=embedded_software;language=all;version=all

Programmer guide:
https://docs.baslerweb.com/pylonapi/cpp/pylon_programmingguide

## 2.3. Samples location
On the Nvidia board
/opt/pylon/share/pylon/Samples/C++

# 3. SSH

At the moment you can connect via ssh:
```bash
ssh -X nvidia@10.0.1.17
```
pwd: nvidia

```bash
ssh -X root@10.0.1.17
```
pwd: icub

# 4. Internet acess to Nvidia board via Shorewall

:exclamation:<u>To be done on iCub-head.</u>

Check and modify in file (you can find it in the repository) `shorewall/interfaces`

- internet access netcard (ZONE=net) with your internet card
- local access netcard (ZONE=lan) with your LAN net card

For check netcard names `ifconfig`

Do the same in `shorewall/masq` \<internet card\>\<lan card\>

Then

```
sudo apt-get install shorewall
sudo cp shorewall/* /etc/shorewall
sudo service shorewall start
```

:exclamation:<u>To be done on Nvidia.</u>

Configure the board address via GUI:
static
ip:10.0.1.17
netmask:255.255.255.0
gateway:10.0.1.104

:exclamation:<u>Test</u>

Test from Nvidia `ping 8.8.8.8`


:warning:_Troubleshooting_

- Check if the Nvidia is running and is connected. 
- Check Nvidia address
- Check if eth board on icub-head is correctly configured