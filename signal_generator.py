# Use the sounddevice module http://python-sounddevice.readthedocs.io/en/0.3.10/

#  _      _______ ____  _____ _____
# | |    |  ____/ ____|/ ____|_   _|
# | |    | |__ | |  __| |  __  | |  
# | |    |  __|| | |_ | | |_ | | |  
# | |____| |___| |__| | |__| |_| |_ 
# |______|______\_____|\_____|_____|
#
# venv IOT_project
# VA RIATTIVATO IL VENV OGNI VOLTA!
# attiva il venv così: 
# source IOT_project/bin/activate
# Poi:
# cd /home/damino/Documenti/PlatformIO/Projects/IotIndividual/
# python3 signal_generator.py

import numpy as np
import sounddevice as sd
import time

samples_per_second = 44100

# Frequency / pitch
freq_hz = 3000

# Duration
duration_s = 60.0

# Attenuation so the sound is reasonable
atten = 0.7 # 0.3


each_sample_number = np.arange(duration_s * samples_per_second)

waveform = np.sin(2 * np.pi * each_sample_number * freq_hz / samples_per_second)

waveform_quiet = waveform * atten


# Play the waveform out the speakers
sd.play(waveform_quiet, samples_per_second)
time.sleep(duration_s)
sd.stop()