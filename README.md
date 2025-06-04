# music_player
A GUI application to manage music. Play songs and manage playlists. Tested on ubuntu 24.04.
The program uses the GUI library GTK4 (https://docs.gtk.org/gtk4/) which includes a widget for playing audio files.
To run the music player you should have your music files already in ~/Music ready for the player to play and manage.
A directory called "music_player_playlists" must exist in the ~/Music directory containing the image files "prev1.png" and "next1.png"
that are contained in this repository.
You can then compile and link main.c into an executable called music_player using the command:
  sudo gcc $( pkg-config --cflags gtk4 ) -o music_player main.c $( pkg-config --libs gtk4 )
The executable can be run from any directory.
