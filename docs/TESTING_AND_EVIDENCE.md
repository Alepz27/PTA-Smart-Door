# Pengujian dan Bukti

Dokumen ini membezakan bukti yang boleh disahkan melalui repositori daripada perkara yang masih memerlukan rekod ujian fizikal bertarikh.

## Asas Bukti

| Pernyataan | Bukti | Status |
| --- | --- | --- |
| Kes simulasi akses RFID dibenarkan dan ditolak tersedia | Projek Proteus, panduan simulasi dan matriks UID | Bukti simulasi didokumenkan |
| Laluan pengesahan RFID dan cap jari wujud | Perisian tegar yang diterbitkan | Disahkan melalui kod sumber |
| Penderia sentuh memanggil laluan akses dibenarkan | Fungsi `checkTouch()` | Disahkan melalui kod sumber |
| Geganti kembali kepada `LOW` selepas kira-kira lima saat | `DOOR_UNLOCK_TIME = 5000` dan `updateDoor()` | Disahkan melalui kod sumber |
| Rekod pengguna disimpan selepas mula semula | Pelaksanaan muat dan simpan melalui `Preferences` | Disahkan melalui kod sumber |
| Log terkini terhad kepada 10 entri dalam RAM | `MAX_LOGS = 10` dan tatasusunan dalam memori | Disahkan melalui kod sumber |
| Akses setempat tidak bergantung pada Wi-Fi atau awan | Gelung perkakasan dipisahkan daripada tugas rangkaian | Disahkan melalui seni bina kod; ujian gangguan fizikal belum direkodkan |
| Rutin pemulihan RC522 dan AS608 wujud | Permulaan semula RC522 berjadual dan penyegerakan semula UART2 | Disahkan melalui kod sumber |
| Semua aliran perkakasan bersepadu lulus | Tiada laporan ujian bertarikh atau video yang sesuai | Memerlukan pengesahan pengguna |
| Pemulihan selepas voltan jatuh atau gangguan kuasa | Tiada rekod ujian terkawal | Belum disahkan |

## Semakan RFID Proteus yang Boleh Diulangi

Ikuti [panduan simulasi](PROTEUS_SIMULATION_GUIDE.md) dan catat keputusan yang diperhatikan. Jangan anggap keputusan dijangka sebagai bukti ujian telah lulus.

| Kes | Input | Keputusan dijangka | Rekod dalam repositori |
| --- | --- | --- | --- |
| Pengguna ALI | `E280689401A9` | Akses dibenarkan dan petunjuk buka kunci | Vektor ujian didokumenkan |
| Pengguna ABU | `E2000019060C` | Akses dibenarkan dan petunjuk buka kunci | Vektor ujian didokumenkan |
| Kad tidak dikenali | `123456789ABC` | Akses ditolak dan pintu kekal berkunci | Vektor ujian didokumenkan |

## Matriks Pengesahan Fizikal yang Disyorkan

Jadual berikut ialah pelan ujian, bukan pernyataan bahawa ujian telah lulus.

| Ujian | Kaedah | Bukti penerimaan yang perlu direkodkan |
| --- | --- | --- |
| Keadaan selamat ketika mula sejuk | Hidupkan prototaip lengkap daripada keadaan tanpa kuasa | Keadaan geganti/kunci, urutan LCD dan log bersiri |
| Akses RFID dibenarkan | Gunakan kad berdaftar | Maklum balas pengguna, tempoh buka kunci dan kunci semula |
| Akses RFID ditolak | Gunakan kad tidak dikenali | Keadaan berkunci dan maklum balas penolakan |
| Cap jari dibenarkan dan ditolak | Gunakan jari berdaftar dan tidak dikenali | Keputusan padanan, keadaan penggerak dan maklum balas |
| Permintaan keluar | Aktifkan TTP223 sekali dan berulang kali | Tingkah laku nyahlantun dan kunci semula selepas lima saat |
| Akses setempat luar talian | Putuskan Wi-Fi sebelum ujian | Laluan setempat kekal responsif; fungsi awan tidak tersedia |
| Pemulihan Wi-Fi | Pulihkan rangkaian yang dikonfigurasi | Masa penyambungan semula dan status Blynk/Telegram |
| Gangguan RC522 | Gunakan kaedah gangguan makmal yang selamat dan ditetapkan | Percubaan pemulihan dan bacaan pulih tanpa mula semula ESP32 |
| Permulaan/pemulihan AS608 | Kitar kuasa mengikut prosedur yang selamat | Percubaan terhad, penyegerakan semula dan padanan pulih |
| Input tidak sah dan aliran pentadbiran | Gunakan ID tidak sah, batalkan pendaftaran dan sekat pengguna | Tiada pemadaman atau buka kunci tanpa kebenaran |
| Gangguan kuasa | Putus dan pulihkan bekalan secara terkawal | Keadaan kunci fizikal, rekod kekal dan mula semula yang teratur |
| Ujian operasi jangka panjang | Jalankan untuk tempoh dan bilangan peristiwa yang ditetapkan | Bilangan mula semula, trend memori dan peristiwa terlepas |

## Bukti yang Masih Diperlukan

- Foto prototaip fizikal dan mekanisme kunci yang telah disemak dari sudut privasi.
- Tangkap layar Blynk dan Telegram selepas identiti serta token disunting keluar.
- Log ujian sistem bersepadu yang bertarikh.
- Ukuran voltan/arus dan belanjawan kuasa.
- Rekod gangguan serta pemulihan terkawal.
- Video demonstrasi yang sesuai.

Tambahkan bukti hanya selepas menyemaknya untuk nama, wajah, alamat e-mel, maklumat rangkaian, token, kata laluan, ID sembang dan maklumat peribadi lain.
