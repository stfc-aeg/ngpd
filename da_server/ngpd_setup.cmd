#set-string "data_dir" "/home/FPuser/ngpd/ADQ14/ngpd_daq/data/"
#set-string "data_dir" "/media/usbdisk/data/"
ngpd config 1 debug
macro "scope" "ngpd start 0 -1.0 read"

ngpd filter generate 0 0 exp 2.5

ngpd setup-diff-trigger 0 0 4000 6 25 14 0 1 0 91
ngpd setup-base-sub 0 0 div-1k
ngpd setup-measure 0 0 20 55 0.1

ngpd setup-neutron-disc 0 -1 475*64 710*64 tail-thres-m 1.4 tail-thres-c 60*90 ignore-fall

ngpd setup-hist 0 nbins-tail-sum 2048 nbins-tail-ratio 2048 max-ratio 0.6 separate-ngp

ngpd adc-board set-input-range 0 0 1 114
ngpd adc-board set-offset 0 0 1 32000

ngpd set-dae-pulse 0 0 20