/*=============================='
 *
 * File: main.c
 * Content: music player source code
 * Date: 2/2/2025
 *
 * compile and link:
 * sudo gcc $( pkg-config --cflags gtk4 ) -o music_player main.c $( pkg-config --libs gtk4 )
 *******************************/

#include <gtk/gtk.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

struct playlist_node
{
	struct playlist_node *succ;
	struct playlist_node *prev;
	char *song;
};

struct play_playlist_data;
struct create_new_playlist_data;
struct browse_machine_to_play_data;
struct add_song_to_playlist_data;

struct browse_machine_to_play_data
{
        GtkFileDialog *fd;
	GtkMediaFile *mf;
        GtkMediaControls *mc;
	GtkWidget *label;
	struct play_playlist_data *ppd;
};

struct create_new_playlist_data
{
	GtkWidget *main_grid;
	GtkWidget *subgrid;
	GtkWidget *playlist_textview;
        GtkWidget *playlist_add_song_button;
        GtkWidget *save_playlist_button;
        GtkWidget *playlist_name_input;
        GtkWidget *playlist_name_prompt;
	struct play_playlist_data *ppd;
	struct edit_playlist_data *epd;
};

struct add_song_to_playlist_data
{
	GtkWidget *playlist_textview;
	GtkFileDialog* file_dialog;
};

struct play_playlist_data
{
	GtkWidget *main_grid;
	GtkWidget *subgrid;
	GtkWidget *playlist_name;
	GtkWidget *play_playlist_textview;
        GtkWidget *select_playlist_to_play;
        GtkWidget *play_playlist_button;
	GtkWidget *file_dialog;
	GtkWidget *media_file;
	GtkWidget *media_controls;
	GtkWidget *current_song;
	struct playlist_node *playlist;
	struct create_new_playlist_data *cpd;
	struct edit_playlist_data *epd;
};

struct edit_playlist_data
{
	GtkWidget *main_grid;
	GtkWidget *subgrid;
	GtkWidget *playlist_name_prompt;
	GtkWidget *playlist_name_input;
	GtkWidget *playlist_textview;
	GtkWidget *select_playlist;
	GtkWidget *add_song_button;
	GtkWidget *save_playlist_button;
	struct play_playlist_data *ppd;
	struct create_new_playlist_data *cpd;
};

void close_dialog_prompt_cb(GtkDialog *self, gpointer user_data)
{
	gtk_window_destroy(self);
}

void file_selector_cb(GObject *source_object,
		      GAsyncResult *res,
		      gpointer bm)
{
	struct browse_machine_to_play_data *browse = (struct browse_machine_to_play_data *)bm;
	GtkFileDialog *file_dialog = browse->fd;
	GError *error = NULL;
	GFile *file_chosen = gtk_file_dialog_open_finish(file_dialog,
				    res,
				    &error);
	if (error != NULL) {
		g_print("AN error occurred.\n");
	}
	if (file_chosen == NULL) {
		g_print("No file chosen\n");
	}
	gtk_media_file_clear(browse->mf);
	gtk_media_file_set_file(browse->mf, file_chosen);
	GCancellable *cancel = g_cancellable_new();
	GFileInfo *file_info = g_file_query_info(file_chosen,
			  			 G_FILE_ATTRIBUTE_STANDARD_DISPLAY_NAME,
			  			 G_FILE_QUERY_INFO_NONE,
			  			 cancel,
			  			 &error);
	g_clear_object(&cancel);
	if (error != NULL) {
		g_print("Error occurred getting chosen file information.\n");
		exit(EXIT_FAILURE);
	}
	const char *file_display_name = g_file_info_get_display_name(file_info);
	int file_display_name_len = strlen(file_display_name);
	char *label_text_prefix = "Current song:  ";
	int label_text_prefix_len = strlen(label_text_prefix);
	char *label_text = (char *)malloc(file_display_name_len + label_text_prefix_len + 1);
       	if (label_text == NULL) {
		printf("Error allocating memory for label text.\n");
		exit(EXIT_FAILURE);
	}
	memcpy(label_text, label_text_prefix, label_text_prefix_len);
	memcpy(&(label_text[label_text_prefix_len]), file_display_name,
	       file_display_name_len);
	label_text[label_text_prefix_len+file_display_name_len] = 0;
	gtk_label_set_text(browse->label, 
			   label_text);
	free(label_text);
}

void activate_browse_to_play(GSimpleAction *action,
                             GVariant *parameter,
                             gpointer bm)
{
	struct browse_machine_to_play_data *browse = (struct browse_machine_to_play_data *)bm;
	GtkFileDialog *f_dialog = browse->fd;
	GCancellable *cancel = g_cancellable_new();
	gtk_file_dialog_open(f_dialog,
			     NULL,
			     cancel,
			     file_selector_cb,
		     	     browse);	 
      	g_clear_object(&cancel);			    	
}
void create_new_playlist(GSimpleAction *action,
                             GVariant *parameter,
                             gpointer data)
{
	struct create_new_playlist_data *cpd = (struct create_new_playlist_data *)data;	
	gtk_widget_hide((cpd->ppd)->subgrid);
	gtk_widget_hide((cpd->epd)->subgrid);
	gtk_widget_show(cpd->subgrid);
}

void edit_playlist(GSimpleAction *action,
                   GVariant *parameter,
                   gpointer data)
{
	struct edit_playlist_data *epd = (struct edit_playlist_data *)data;
        GtkGrid *subgrid = epd->subgrid;
        gtk_widget_hide((epd->cpd)->subgrid);
	gtk_widget_hide((epd->ppd)->subgrid);
        gtk_widget_show(subgrid);
}

void add_song_to_new_playlist_cb(GObject *source_object,
                      GAsyncResult *res,
                      gpointer user_data)
{
	struct add_song_to_playlist_data *asd = (struct add_song_to_playlist_data *)user_data;
	GtkFileDialog *file_dialog = asd->file_dialog;
        GError *error = NULL;
        GFile *file_chosen = gtk_file_dialog_open_finish(file_dialog,
                                    			 res,
                                    			 &error);
        if (error != NULL) {
		if ((error->code) == 2) { // user cancelled the selection of a file
                	g_print("%s. User cancelled selection of file.\n", error->message);
			return;
		}
        }
        if (file_chosen == NULL) {
                g_print("No file chosen\n");
		exit(EXIT_FAILURE);
        }	
	GCancellable *cancel = g_cancellable_new();
        GFileInfo *file_info = g_file_query_info(file_chosen,
                                                 G_FILE_ATTRIBUTE_STANDARD_DISPLAY_NAME,
                                                 G_FILE_QUERY_INFO_NONE,
                                                 cancel,
                                                 &error);
	g_clear_object(&cancel);
        if (error != NULL) {
                g_print("Error occurred getting chosen file information.\n");
                exit(EXIT_FAILURE);
        }
        const char *file_display_name = g_file_info_get_display_name(file_info);
	GtkTextBuffer *curr_textbuffer = gtk_text_view_get_buffer(asd->playlist_textview);
	GtkTextIter start_iter;
	GtkTextIter end_iter;
	gtk_text_buffer_get_start_iter(curr_textbuffer, &start_iter);
	gtk_text_buffer_get_end_iter(curr_textbuffer, &end_iter);
	char *buffer_text = gtk_text_iter_get_text(&start_iter, &end_iter);
	int newline_count = 0;
	int index = 0;
	while (buffer_text[index]!='\0') {
		if (buffer_text[index] == '\n') {
			++newline_count;
		}
		++index;
	}
	int buffer_text_len = strlen(buffer_text);
	char *new_text_buffer = malloc(buffer_text_len + 10 + strlen(file_display_name));
	if (new_text_buffer == NULL) {
		printf("Error allocating memory on heap using malloc.");
		exit(EXIT_FAILURE);
	}
	memset(new_text_buffer, 0, buffer_text_len + 10 + strlen(file_display_name));
	if (buffer_text_len == 0) {
		++newline_count;
		sprintf(new_text_buffer, "%d - %s", newline_count, file_display_name);
	}	
	else {
		newline_count += 2;
		sprintf(new_text_buffer, "%s\n%d - %s",buffer_text, newline_count, file_display_name);
	}	
	gtk_text_buffer_set_text(curr_textbuffer, new_text_buffer, -1);
	free(buffer_text);
}

void
add_song_to_playlist_cb (
  GtkButton* add_song_button,
  gpointer user_data)
{
	struct add_song_to_playlist_data *asd = (struct add_song_to_playlist_data *)user_data;
	GtkFileDialog *f_dialog = asd->file_dialog;
        GCancellable *cancel = g_cancellable_new();
        gtk_file_dialog_open(f_dialog,
                             NULL,
                             cancel,
                             add_song_to_new_playlist_cb,
                             asd);
	g_clear_object(&cancel);
	
}

void add_song_to_edited_playlist_cb(GObject *source_object,
                      GAsyncResult *res,
                      gpointer user_data)
{
	struct add_song_to_playlist_data *asd = (struct add_song_to_playlist_data *)user_data;
        GtkFileDialog *file_dialog = asd->file_dialog;
        GError *error = NULL;
        GFile *file_chosen = gtk_file_dialog_open_finish(file_dialog,
                                                         res,
                                                         &error);
        if (error != NULL) {
        	 if ((error->code) == 2) { // user cancelled the selection of a file
                        g_print("%s. User cancelled selection of file.\n", error->message);
                        return;
                }
	}
        if (file_chosen == NULL) {
                g_print("No file chosen\n");
                exit(EXIT_FAILURE);
        }
        GCancellable *cancel = g_cancellable_new();
        GFileInfo *file_info = g_file_query_info(file_chosen,
                                                 G_FILE_ATTRIBUTE_STANDARD_DISPLAY_NAME,
                                                 G_FILE_QUERY_INFO_NONE,
                                                 cancel,
                                                 &error);
	g_clear_object(&cancel);
        if (error != NULL) {
                g_print("Error occurred getting chosen file information.\n");
                exit(EXIT_FAILURE);
        }
        const char *file_display_name = g_file_info_get_display_name(file_info);
        GtkTextBuffer *curr_textbuffer = gtk_text_view_get_buffer(asd->playlist_textview);
        GtkTextIter start_iter;
        GtkTextIter end_iter;
        gtk_text_buffer_get_start_iter(curr_textbuffer, &start_iter);
        gtk_text_buffer_get_end_iter(curr_textbuffer, &end_iter);
        char *buffer_text = gtk_text_iter_get_text(&start_iter, &end_iter);
        int newline_count = 0;
        int index = 0;
	while (buffer_text[index]!='\0') {
                if (buffer_text[index] == '\n') {
                        ++newline_count;
                }
                ++index;
        }
        int buffer_text_len = strlen(buffer_text);
        char *new_text_buffer = malloc(buffer_text_len + 10 + strlen(file_display_name));
        if (new_text_buffer == NULL) {
		printf("Error allocating memory using malloc.");
		exit(EXIT_FAILURE);
        }
        memset(new_text_buffer, 0, buffer_text_len + 10 + strlen(file_display_name));
        if (buffer_text_len == 0) {
                ++newline_count;
                sprintf(new_text_buffer, "%s", file_display_name);
        }
        else {
                newline_count += 2;
                sprintf(new_text_buffer, "%s\n%s",buffer_text, file_display_name);
        }
        gtk_text_buffer_set_text(curr_textbuffer, new_text_buffer, -1);
	free(buffer_text);
}

void
add_song_to_playlist_cb2 (
  GtkButton* add_song_button,
  gpointer user_data)
{
        struct add_song_to_playlist_data *asd = (struct add_song_to_playlist_data *)user_data;
        GtkFileDialog *f_dialog = asd->file_dialog;
        GCancellable *cancel = g_cancellable_new();
        gtk_file_dialog_open(f_dialog,
                             NULL,
                             cancel,
                             add_song_to_edited_playlist_cb,
                             asd);
	g_clear_object(&cancel);
}

void save_playlist_cb( GtkButton *save_playlist_button,
		       gpointer user_data)
{
	struct create_new_playlist_data *npd = (struct create_new_playlist_data *)user_data;
	GtkEntryBuffer *entry_buffer = gtk_text_get_buffer(npd->playlist_name_input);
	const char *playlist_name = gtk_entry_buffer_get_text(entry_buffer);
	if (*playlist_name == '\0') {
		// empty playlist name - let user know
		GtkWidget *dialog_prompt = gtk_dialog_new();
		g_signal_connect(dialog_prompt, "close-request", close_dialog_prompt_cb, NULL);
		GtkWidget *content_area = gtk_dialog_get_content_area(dialog_prompt);
		GtkWidget *label = gtk_label_new("\n\nMissing playlist title.\nPlease input a playlist title.\n\n");
		gtk_box_append(GTK_BOX(content_area), label);
		gtk_widget_show(dialog_prompt);
	}
	else {
		int file_exists = access(playlist_name, F_OK);
		if (file_exists == 0) {
			GtkWidget *dialog_prompt = gtk_dialog_new();
			g_signal_connect(dialog_prompt, "close-request", close_dialog_prompt_cb, NULL);
			GtkWidget *content_area = gtk_dialog_get_content_area(dialog_prompt);
			GtkWidget *label = gtk_label_new("\n\nPlaylist name already exists.\nPlease choose a different name for this playlist.\n\n");
			gtk_box_append(GTK_BOX(content_area), label);
			gtk_widget_show(dialog_prompt);
		}
		else {
			FILE *f = fopen(playlist_name, "w+");
			if (f == NULL) {
				printf("Error opening playlist file %s for writing. %s\n", playlist_name, strerror(errno));
				exit(EXIT_FAILURE);
			}

			GtkTextBuffer *text_buffer = gtk_text_view_get_buffer(npd->playlist_textview);
			GtkTextIter start_iter;
			GtkTextIter end_iter;
			gtk_text_buffer_get_start_iter(text_buffer, &start_iter);
			gtk_text_buffer_get_end_iter(text_buffer, &end_iter);
			char *playlist_text = gtk_text_buffer_get_text(text_buffer,
						 &start_iter,
						 &end_iter,
						 true);
			int track_name_len = strlen(playlist_text);
			char *curr_track_name = (char *)malloc(track_name_len+2);
			if (curr_track_name == NULL) {
				printf("Error allocating memory for track names.\n");
				exit(EXIT_FAILURE);
			}
			int index0 = 0;
			int index1 = 0;
			int curr_track_name_index = 0;
			uint8_t error = 0;
			while (playlist_text[index0] != '\0') {
				if (index0 > 0) {
 					++index0;
				}
				index1 = 0;
				if ( (playlist_text[index0] <'0') && 
				     (playlist_text[index0] >'9') ) {
					error = 1;
					break;
				}
				++index0;
				const char *prefix = " - ";
				if ( (playlist_text[index0] != ' ') ||
				     (playlist_text[index0+1] != '-') ||
				     (playlist_text[index0+2] != ' ') )	{
					error = 1;
					break;
				}	
				index0 += 3;
				while ( ( playlist_text[index0] != '\0') &&
				        ( playlist_text[index0] != '\n') ) {
					curr_track_name[index1++] = playlist_text[index0++];
				}
				curr_track_name[index1++] = '\n';
				curr_track_name[index1] = 0;
				if ( fwrite(curr_track_name,
				            1, index1, f) != index1) {
					printf("Error writing part of playlist to disk.%s.\n", strerror(errno));
					exit(EXIT_FAILURE);
				}
				if (fflush(f) != 0) {
                                        printf("Error writing new playlist to disk.\n", strerror(errno));
                                        exit(EXIT_FAILURE);
                                }
			}
			if (error==1) {
				GtkWidget *dialog_prompt = gtk_dialog_new();
				g_signal_connect(dialog_prompt, "close-request", close_dialog_prompt_cb, NULL);
	                        GtkWidget *content_area = gtk_dialog_get_content_area(dialog_prompt);
        	                GtkWidget *label = gtk_label_new("\n\nError in playlist input format.\nPlease try to create the playlist again.\n\n");
                	        gtk_box_append(GTK_BOX(content_area), label);
                        	gtk_widget_show(dialog_prompt);
				if ( chdir("../") == -1) {
					printf("Error moving in directory structure. %s.\n", strerror(errno));
					exit(EXIT_FAILURE);
				}
			}
			GtkWidget *dialog_prompt = gtk_dialog_new();
			g_signal_connect(dialog_prompt, "close-request", close_dialog_prompt_cb, NULL);
			GtkWidget *content_area = gtk_dialog_get_content_area(dialog_prompt);
			GtkWidget *label = gtk_label_new("\n\nPlaylist successfully saved.\n\n");
			gtk_box_append(GTK_BOX(content_area), label);
			gtk_widget_show(dialog_prompt);
			free(curr_track_name);
			if ( fclose(f) == EOF) {
				printf("Error occurred whilst closing file stream associated with new playlist.\n%s",
				       strerror(errno));
				exit(EXIT_FAILURE);
			}
		}
	}	
}

void save_edited_playlist_cb(GtkButton *save_playlist_button,
    	                     gpointer user_data)
{
	struct edit_playlist_data *epd = (struct edit_playlist_data *)user_data;
        GtkEntryBuffer *entry_buffer = gtk_text_get_buffer(epd->playlist_name_input);
        const char *playlist_name = gtk_entry_buffer_get_text(entry_buffer);
        if (*playlist_name == '\0') {
                // empty playlist name - let user know
                GtkWidget *dialog_prompt = gtk_dialog_new();
		g_signal_connect(dialog_prompt, "close-request", close_dialog_prompt_cb, NULL);
                GtkWidget *content_area = gtk_dialog_get_content_area(dialog_prompt);
                GtkWidget *label = gtk_label_new("\n\nMissing playlist title.\nPlease input a playlist title.\n\n");
                gtk_box_append(GTK_BOX(content_area), label);
                gtk_widget_show(dialog_prompt);
        }
        else {
                int file_exists = access(playlist_name, F_OK);
		FILE *f = fopen(playlist_name, "w");
		if (f == NULL) {
			printf("Error opening playlist file %s for writing. %s\n", playlist_name, strerror(errno));
			exit(EXIT_FAILURE);
		}

		GtkTextBuffer *text_buffer = gtk_text_view_get_buffer(epd->playlist_textview);
		GtkTextIter start_iter;
		GtkTextIter end_iter;
		gtk_text_buffer_get_start_iter(text_buffer, &start_iter);
		gtk_text_buffer_get_end_iter(text_buffer, &end_iter);
		char *playlist_text = gtk_text_buffer_get_text(text_buffer,
					 &start_iter,
					 &end_iter,
					 true);
		int track_name_len = strlen(playlist_text);
		char *curr_track_name = (char *)malloc(track_name_len+2);
		if (curr_track_name == NULL) {
			printf("Error allocating memory for track names.\n");
			exit(EXIT_FAILURE);
		}
		int index0 = 0;
		int index1 = 0;
		int curr_track_name_index = 0;
		uint8_t error = 0;
		while (playlist_text[index0] != '\0') {
			if (index0 > 0) {
				++index0;
			}
			index1 = 0;
			while ( ( playlist_text[index0] != '\0') &&
				( playlist_text[index0] != '\n') ) {
				curr_track_name[index1++] = playlist_text[index0++];
			}
			curr_track_name[index1++] = '\n';
			curr_track_name[index1] = 0;
			if (index1 > 1) {
				if ( fwrite(curr_track_name,
					    1, index1, f) != index1) {
					printf("Error writing part of playlist to disk.%s.\n", strerror(errno));
					exit(EXIT_FAILURE);
				}
				if (fflush(f) != 0) {
					printf("Error writing edited playlist to disk.\n", strerror(errno));
					exit(EXIT_FAILURE);
				}
			}
		}
		if (error==1) {
			GtkWidget *dialog_prompt = gtk_dialog_new();
			g_signal_connect(dialog_prompt, "close-request", close_dialog_prompt_cb, NULL);
			GtkWidget *content_area = gtk_dialog_get_content_area(dialog_prompt);
			GtkWidget *label = gtk_label_new("\n\nError in playlist input format.\nPlease try to create the playlist again.\n\n");
			gtk_box_append(GTK_BOX(content_area), label);
			gtk_widget_show(dialog_prompt);
			if ( chdir("../") == -1) {
				printf("Error moving in directory structure. %s.\n", strerror(errno));
				exit(EXIT_FAILURE);
			}
		}
		GtkWidget *dialog_prompt = gtk_dialog_new();
		g_signal_connect(dialog_prompt, "close-request", close_dialog_prompt_cb, NULL);
		GtkWidget *content_area = gtk_dialog_get_content_area(dialog_prompt);
		GtkWidget *label = gtk_label_new("\n\nPlaylist successfully saved.\n\n");
		gtk_box_append(GTK_BOX(content_area), label);
		gtk_widget_show(dialog_prompt);
		free(curr_track_name);
		if (fclose(f)==EOF) {
			printf("Error occurred whilst closing stream associated with writing edited playlist to disk.\n%s",
				strerror(errno));
			exit(EXIT_FAILURE);
		}
	}         	
}

void select_playlist_asynch_cb(GObject *file_dialog,
			       GAsyncResult *res,
			       gpointer data)
{
	struct play_playlist_data *ppd = (struct play_playlist_data *)data;
	GError *error = NULL;
	GFile *f = gtk_file_dialog_open_finish(file_dialog, 
					res, 
					&error);
	if (error != NULL) {
                 if ((error->code) == 2) { // user cancelled the selection of a file
                        g_print("%s. User cancelled selection of file.\n", error->message);
                        return;
                }
        }
	GCancellable *cancel = g_cancellable_new();
        error = NULL;
	GFileInfo *file_info = g_file_query_info(f,
                                                 G_FILE_ATTRIBUTE_STANDARD_DISPLAY_NAME,
                                                 G_FILE_QUERY_INFO_NONE,
                                                 cancel,
                                                 &error);

	g_clear_object(&cancel);
	if (error != NULL) {
		g_print("An error occurred whilst selecting the playlist to play. %s", error->message);
		exit(EXIT_FAILURE);
	}
	if (file_info ==  NULL) {
		printf("Error getting info of playlist file to play.\n");
		exit(EXIT_FAILURE);
	}
	char *playlist_name = g_file_info_get_display_name(file_info);
	gtk_label_set_label(ppd->playlist_name, playlist_name);
	FILE *file = fopen(playlist_name, "r");
       	if (file == NULL) {
		printf("Error opening playlist file %s.%s\n",playlist_name, strerror(errno));
		exit(EXIT_FAILURE);
	}	
	if (fseek(file, 0, SEEK_END)==-1) {
		printf("Error seeking to end of playlist file %s.%s.\n", playlist_name, strerror(errno));
		exit(EXIT_FAILURE);
	}
	long playlist_len = ftell(file);
	char *playlist_file_buffer = (char *)malloc(playlist_len+1);
	if (playlist_file_buffer == NULL) {
		printf("Error allocating memory to store playlist file.\n");
		exit(EXIT_FAILURE);
	}
	if (fseek(file, 0, SEEK_SET)==-1) {
		printf("Error seeking to start of file.%s.\n", strerror(errno));
		exit(EXIT_FAILURE);
	}
	int fres = fread(playlist_file_buffer, 1, playlist_len, file);
	if (fres < playlist_len)	{
		printf("Error reading playlist into RAM from disk.\n");
		printf("playlist_len:%d  res:%d\n", playlist_len, fres);
		exit(EXIT_FAILURE);
	}
	playlist_file_buffer[playlist_len] = 0;
	GtkTextBuffer *text_buffer = gtk_text_view_get_buffer(ppd->play_playlist_textview);
	gtk_text_buffer_set_text(text_buffer, playlist_file_buffer, -1);
	gtk_label_set_text(ppd->playlist_name, playlist_name);
	if ( fclose(file) == EOF) {
		printf("Error closing playlist file.\n");
		exit(EXIT_FAILURE);
	}
	free(playlist_file_buffer);
}

void select_playlist_cb( GtkButton *save_playlist_button,
                       	gpointer user_data)
{
	struct play_playlist_data *ppd = (struct play_playlist_data *)user_data;
	GtkFileDialog *file_dialog = gtk_file_dialog_new();
	GCancellable *cancel = g_cancellable_new();
	gtk_file_dialog_open(file_dialog,
			     NULL,
			     cancel,
			     select_playlist_asynch_cb,
			     ppd);
	g_clear_object(&cancel);
}

void select_playlist_asynch_cb2(GObject *file_dialog,
                               GAsyncResult *res,
                               gpointer data)
{
	struct edit_playlist_data *epd = (struct play_playlist_data *)data;
        GError *error = NULL;
        GFile *f = gtk_file_dialog_open_finish(file_dialog,
                                        res,
                                        &error);

        GCancellable *cancel = g_cancellable_new();
        if (error != NULL) {
                 if ((error->code) == 2) { // user cancelled the selection of a file
                        g_print("%s. User cancelled selection of file.\n", error->message);
                        return;
                }
        }
	error = NULL;
	GFileInfo *file_info = g_file_query_info(f,
                                                 G_FILE_ATTRIBUTE_STANDARD_DISPLAY_NAME,
                                                 G_FILE_QUERY_INFO_NONE,
                                                 cancel,
                                                 &error);
        if (file_info ==  NULL) {
                printf("Error getting info of database file.\n");
                exit(EXIT_FAILURE);
        }
        char *playlist_name = g_file_info_get_display_name(file_info);
        FILE *file = fopen(playlist_name, "r");
        if (file == NULL) {
                printf("Error opening playlist file %s.%s\n",playlist_name, strerror(errno));
                exit(EXIT_FAILURE);
        }
        if (fseek(file, 0, SEEK_END)==-1) {
                printf("Error seeking to end of playlist file %s.%s.\n", playlist_name, strerror(errno));
                exit(EXIT_FAILURE);
        }
	long playlist_len = ftell(file);
        char *playlist_file_buffer = (char *)malloc(playlist_len+1);
        if (playlist_file_buffer == NULL) {
                printf("Error allocating memory to store playlist file.\n");
                exit(EXIT_FAILURE);
        }
        if (fseek(file, 0, SEEK_SET)==-1) {
                printf("Error seeking to start of file.%s.\n", strerror(errno));
                exit(EXIT_FAILURE);
        }
        int fres = fread(playlist_file_buffer, 1, playlist_len, file);
        if (fres < playlist_len)        {
                printf("Error reading playlist into RAM from disk.\n");
                printf("playlist_len:%d  res:%d\n", playlist_len, fres);
                exit(EXIT_FAILURE);
        }
        playlist_file_buffer[playlist_len] = 0;
        GtkTextBuffer *text_buffer = gtk_text_view_get_buffer(epd->playlist_textview);
	gtk_text_buffer_set_text(text_buffer, playlist_file_buffer, -1);

	GtkEntryBuffer *playlist_name_buffer = gtk_text_get_buffer(epd->playlist_name_input);
	gtk_entry_buffer_set_text(playlist_name_buffer, playlist_name, -1);
	gtk_widget_show(epd->playlist_name_input);
	free(playlist_file_buffer);
}

void select_playlist_cb2( GtkButton *save_playlist_button,
                        gpointer user_data)
{
        struct edit_playlist_data *epd = (struct play_playlist_data *)user_data;
        GtkFileDialog *file_dialog = gtk_file_dialog_new();
        GCancellable *cancel = g_cancellable_new();
        gtk_file_dialog_open(file_dialog,
                             NULL,
                             cancel,
                             select_playlist_asynch_cb2,
                             epd);
	g_clear_object(&cancel);
}

void play_playlist_cb( GtkButton *play_playlist_button,
                       gpointer user_data)
{

	struct play_playlist_data *ppd = (struct play_playlist_data *)user_data;
	GtkTextBuffer *text_buffer = gtk_text_view_get_buffer(ppd->play_playlist_textview);
	GtkTextIter start_iter;
	GtkTextIter end_iter;
	gtk_text_buffer_get_start_iter(text_buffer, &start_iter);
	gtk_text_buffer_get_end_iter(text_buffer, &end_iter);
	char *buffer = gtk_text_iter_get_text(&start_iter, &end_iter);
	int buffer_len = strlen(buffer);
	char *curr_song_file_name = 0;
	int index = 0;
	int song_index = 0;
	struct playlist_node *first_song = ppd->playlist;
	struct playlist_node *present_song = first_song;
	if ((present_song->succ) == 0) {
                free(present_song->song);
                present_song->song = 0;
		present_song->succ = 0;
		present_song->prev = 0;
        }
	else {
		while ((present_song->succ) != first_song) {
			if (present_song == first_song) {
				free(present_song->song);
				present_song->song = 0;
				struct playlist_node *temp = present_song->succ;
				present_song->succ = 0;
				present_song->prev = 0;
				present_song = temp;
			}
			else {
				free(present_song->song);
				present_song->song = 0;
				struct playlist_node *temp = present_song->succ;
				free(present_song);
				present_song = temp;	
			}
		}
		free(present_song->song);
		free(present_song);
	}	
	while (buffer[index] != '\0') {
		song_index = 0;
		curr_song_file_name = (char *)malloc(buffer_len + 1);
		if (curr_song_file_name == NULL) {
			printf("Error allocating memory for current song name in playlist.\n");
			exit(EXIT_FAILURE);
		}
		while ((buffer[index] != '\n') && (buffer[index]!='\0')) {
			curr_song_file_name[song_index++] = buffer[index++];
		}
		curr_song_file_name[song_index] = 0;
		(ppd->playlist)->song = curr_song_file_name;
		(ppd->playlist)->succ = (struct playlist_node *)malloc(sizeof(struct playlist_node));
		if ((ppd->playlist)->succ == NULL) {
			printf("Error allocating memory for next playlist_node");
			exit(EXIT_FAILURE);
		}
		((ppd->playlist)->succ)->prev = ppd->playlist;
		((ppd->playlist)->succ)->succ = 0;
		((ppd->playlist)->succ)->song = 0;
		ppd->playlist = (ppd->playlist)->succ;
		if (buffer[index] == '\n') {
			++index;
		} 
	}
	if ((ppd->playlist)->prev == first_song) {
		// playlist only has one song
		free(ppd->playlist);
		ppd->playlist = first_song;
		first_song->succ = 0;
		first_song->prev = 0;
	}
	else if ((first_song->song) != 0) {
		// more than one song in playlist. connect first and last
		((ppd->playlist)->prev)->succ = first_song;
		first_song->prev = (ppd->playlist)->prev;
		free(ppd->playlist);
		ppd->playlist = first_song;
	}
	if ((first_song->song) != 0) {
		// play first song
		char *home_dir = getenv("HOME");
                int home_dir_len = strlen(home_dir);
                char *home_suffix = "/Music";
                int home_suffix_len = strlen(home_suffix);
                char *music_dir = (char *)malloc(home_dir_len + home_suffix_len + 1);
                if (music_dir == NULL) {
                        printf("Error allocating memory to hold ~/Music directory absolute path name.%s.\n", strerror(errno));
                        exit(EXIT_FAILURE);
                }
                memcpy(music_dir, home_dir, home_dir_len);
                memcpy(&(music_dir[home_dir_len]), home_suffix, home_suffix_len);
                music_dir[home_dir_len + home_suffix_len] = 0;
                char *home_suffix2 = "/Music/music_player_playlists";
                int home_suffix2_len = strlen(home_suffix2);
                char *playlists_dir = (char *)malloc(home_dir_len + home_suffix2_len + 1);
                if (playlists_dir == NULL) {
                        printf("Error allocating memory to hold playlists directory absolute path name.%s.\n", strerror(errno));
                        exit(EXIT_FAILURE);
                }
                memcpy(playlists_dir, home_dir, home_dir_len);
                memcpy(&(playlists_dir[home_dir_len]), home_suffix2, home_suffix2_len);
                playlists_dir[home_dir_len + home_suffix2_len] = 0;
                if ( chdir(music_dir) == -1) {
                        printf("Error changing directory to ~/Music. %s.\n",
                                strerror(errno));
                        exit(EXIT_FAILURE);
                }
		GError *error = NULL;
                int song_str_len = strlen(first_song->song);
                gchar *song_file_glib_encoding = g_filename_from_utf8(first_song->song,
                                                        song_str_len,
                                                        NULL,
                                                        NULL,
                                                        &error); 
                if (error != NULL) {
                        printf("Error getting glib encoded file name of song %s. %s.\n",
                               first_song->song, error->message);
                        exit(EXIT_FAILURE);
                }
                g_error_free(error);
		gtk_media_file_clear(ppd->media_file);
		GFile *new_file = g_file_new_for_path(song_file_glib_encoding);
		gtk_media_file_set_file(ppd->media_file, new_file);

		char *current_song_prefix = "Current song: ";
		char *current_song_label = (char *)malloc(strlen(current_song_prefix) + strlen(first_song->song) + 1);
		if (current_song_label == NULL) {
			printf("Error allocating memory to display curent song.\n");
			exit(EXIT_FAILURE);
		}
		memcpy(current_song_label, current_song_prefix, strlen(current_song_prefix));
		memcpy(&(current_song_label[strlen(current_song_prefix)]), first_song->song, strlen(first_song->song));
		current_song_label[strlen(current_song_prefix) + strlen(first_song->song)] = 0;
		gtk_label_set_text(ppd->current_song, current_song_label);
	        if ( chdir(playlists_dir) == -1) {
			printf("Error changing directory to playlists directory. %s.\n", strerror(errno));
			exit(EXIT_FAILURE);
		}
		free(current_song_label);
		free(playlists_dir);
		free(music_dir);
	}
}

void play_playlist(GSimpleAction *action,
                   GVariant *parameter,
                   gpointer data)
{
	struct play_playlist_data *ppd = (struct play_playlist_data *)data;		
	GtkGrid *subgrid = ppd->subgrid;
	gtk_widget_hide((ppd->cpd)->subgrid);
	gtk_widget_hide((ppd->epd)->subgrid);
	gtk_widget_show(subgrid);
}

void prev_song_cb(GtkButton *prev_button,
		  gpointer user_data)
{
	struct play_playlist_data *ppd = (struct play_playlist_data *)user_data;
	if ((ppd->playlist)->prev != 0) {
		char *home_dir = getenv("HOME");
                int home_dir_len = strlen(home_dir);
                char *home_suffix = "/Music";
                int home_suffix_len = strlen(home_suffix);
                char *music_dir = (char *)malloc(home_dir_len + home_suffix_len + 1);
                if (music_dir == NULL) {
                        printf("Error allocating memory to hold ~/Music directory absolute path name.%s.\n", strerror(errno));
                        exit(EXIT_FAILURE);
                }
                memcpy(music_dir, home_dir, home_dir_len);
                memcpy(&(music_dir[home_dir_len]), home_suffix, home_suffix_len);
                music_dir[home_dir_len + home_suffix_len] = 0;
                char *home_suffix2 = "/Music/music_player_playlists";
                int home_suffix2_len = strlen(home_suffix2);
                char *playlists_dir = (char *)malloc(home_dir_len + home_suffix2_len + 1);
                if (playlists_dir == NULL) {
                        printf("Error allocating memory to hold playlists directory absolute path name.%s.\n", strerror(errno));
                        exit(EXIT_FAILURE);
                }
                memcpy(playlists_dir, home_dir, home_dir_len);
                memcpy(&(playlists_dir[home_dir_len]), home_suffix2, home_suffix2_len);
                playlists_dir[home_dir_len + home_suffix2_len] = 0;
                if ( chdir(music_dir) == -1) {
                        printf("Error changing directory to ~/Music. %s.\n",
                                strerror(errno));
                        exit(EXIT_FAILURE);
                }
		ppd->playlist = (ppd->playlist)->prev;
		GError *error = NULL;
                int song_str_len = strlen((ppd->playlist)->song);
                gchar *song_file_glib_encoding = g_filename_from_utf8((ppd->playlist)->song,
                                                        song_str_len,
                                                        NULL,
                                                        NULL,
                                                        &error);
                if (error != NULL) {
                        printf("Error getting glib encoded file name of song %s. %s.\n",
                               (ppd->playlist)->song, error->message);
                        exit(EXIT_FAILURE);
                }
                g_error_free(error);
                GFile *filestream = g_file_new_for_path(song_file_glib_encoding);
		gtk_media_file_clear(ppd->media_file);
		gtk_media_file_set_file(ppd->media_file, filestream);
		char *current_song_prefix = "Current song: ";
                char *current_song_label = (char *)malloc(strlen(current_song_prefix) + strlen((ppd->playlist)->song) + 1);
                if (current_song_label == NULL) {
                        printf("Error allocating memory to display curent song.\n");
                        exit(EXIT_FAILURE);
                }
                memcpy(current_song_label, current_song_prefix, strlen(current_song_prefix));
                memcpy(&(current_song_label[strlen(current_song_prefix)]), (ppd->playlist)->song, strlen((ppd->playlist)->song));
                current_song_label[strlen(current_song_prefix) + strlen((ppd->playlist)->song)] = 0;
                gtk_label_set_text(ppd->current_song, current_song_label);
		free(current_song_label);
		if (chdir(playlists_dir)== -1) {
                        printf("Error changing directory to directory of binary. %s.\n", strerror(errno));
                        exit(EXIT_FAILURE);
                }
		free(playlists_dir);
		free(music_dir);
	}
}

void next_song_cb(GtkButton *next_button,
		  gpointer user_data)
{
	struct play_playlist_data *ppd = (struct play_playlist_data *)user_data;
        if ((ppd->playlist)->succ != 0) {
		char *home_dir = getenv("HOME");
                int home_dir_len = strlen(home_dir);
                char *home_suffix = "/Music";
                int home_suffix_len = strlen(home_suffix);
                char *music_dir = (char *)malloc(home_dir_len + home_suffix_len + 1);
                if (music_dir == NULL) {
                        printf("Error allocating memory to hold ~/Music directory absolute path name.%s.\n", strerror(errno));
                        exit(EXIT_FAILURE);
                }
                memcpy(music_dir, home_dir, home_dir_len);
                memcpy(&(music_dir[home_dir_len]), home_suffix, home_suffix_len);
                music_dir[home_dir_len + home_suffix_len] = 0;
                char *home_suffix2 = "/Music/music_player_playlists";
                int home_suffix2_len = strlen(home_suffix2);
                char *playlists_dir = (char *)malloc(home_dir_len + home_suffix2_len + 1);
                if (playlists_dir == NULL) {
                        printf("Error allocating memory to hold playlists directory absolute path name.%s.\n", strerror(errno));
                        exit(EXIT_FAILURE);
                }
                memcpy(playlists_dir, home_dir, home_dir_len);
                memcpy(&(playlists_dir[home_dir_len]), home_suffix2, home_suffix2_len);
                playlists_dir[home_dir_len + home_suffix2_len] = 0;
                if ( chdir(music_dir) == -1) {
                        printf("Error changing directory to ~/Music. %s.\n",
                                strerror(errno));
                        exit(EXIT_FAILURE);
                }
                ppd->playlist = (ppd->playlist)->succ;
		if (chdir(music_dir)==-1) {
			printf("Error changing directory to ~/Music directory.%s.\n", strerror(errno));
			exit(EXIT_FAILURE);
		}
		GError *error = NULL;
		int song_str_len = strlen((ppd->playlist)->song);
		gchar *song_file_glib_encoding = g_filename_from_utf8((ppd->playlist)->song,
                                                        song_str_len,
                                                        NULL,
                                                        NULL,
                                                        &error);
	        if (error != NULL) {
         	        printf("Error getting glib encoded file name of song %s. %s.\n",
                	       (ppd->playlist)->song, error->message);
                	exit(EXIT_FAILURE);
        	}
        	g_error_free(error);
        	GFile *filestream = g_file_new_for_path(song_file_glib_encoding);
		gtk_media_file_clear(ppd->media_file);
		gtk_media_file_set_file(ppd->media_file, filestream);
                char *current_song_prefix = "Current song: ";
                char *current_song_label = (char *)malloc(strlen(current_song_prefix) + strlen((ppd->playlist)->song) + 1);
                if (current_song_label == NULL) {
                        printf("Error allocating memory to display curent song.\n");
                        exit(EXIT_FAILURE);
                }
                memcpy(current_song_label, current_song_prefix, strlen(current_song_prefix));
                memcpy(&(current_song_label[strlen(current_song_prefix)]), (ppd->playlist)->song, strlen((ppd->playlist)->song));
                current_song_label[strlen(current_song_prefix) + strlen((ppd->playlist)->song)] = 0;
                gtk_label_set_text(ppd->current_song, current_song_label);
		free(current_song_label);
		if (chdir(playlists_dir)== -1) {
                        printf("Error changing directory to directory of binary. %s.\n", strerror(errno));
                        exit(EXIT_FAILURE);
                }
		free(music_dir);
		free(playlists_dir);
        }
}

static void
activate (GtkApplication *app,
          gpointer        user_data)
{
	// edit playlist, then play playlist, then browse machine, then cnp causes error
	GtkWidget *window;
	GtkWidget *grid;
	GtkWidget *browse_machine_button;
	GtkWidget *playlists_menu_button;
	GtkWidget *current_song;
	GtkWidget *playlist_textview;
        GtkWidget *playlist_add_song_button;
	GtkWidget *save_playlist_button;
	GtkWidget *playlist_name_input;
	GtkWidget *playlist_name_prompt;
	GtkWidget *create_new_playlist_grid;

	GtkWidget *play_playlist_textview;
	GtkWidget *select_playlist_to_play;
	GtkWidget *play_playlist_button;
	GtkWidget *playlist_name;
	GtkWidget *play_playlist_grid;

	GtkWidget *create_playlist_grid;
	create_playlist_grid = gtk_grid_new();

	GtkWidget *menu_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
	GtkWidget *playlist_track_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
	GtkWidget *prev_button = gtk_button_new();	
	GtkBox *prev_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
	GtkImage *prev_image = gtk_image_new_from_file("prev1.png");
	gtk_box_append(prev_box, prev_image);
	gtk_button_set_child(prev_button, prev_box);
	gtk_widget_set_halign(prev_button, GTK_ALIGN_END);
	GtkWidget *next_button = gtk_button_new();
	GtkImage *next_image = gtk_image_new_from_file("next1.png");
	GtkBox *next_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
	gtk_box_append(next_box, next_image);
	gtk_button_set_child(next_button, next_box);
	gtk_widget_set_halign(next_button, GTK_ALIGN_END);
	gtk_box_append(playlist_track_box, prev_button);
	gtk_box_append(playlist_track_box, next_button);
	play_playlist_grid = gtk_grid_new();

	
	GtkFileDialog *file_dialog = gtk_file_dialog_new();
	current_song = gtk_label_new("Current song: ");
	grid = gtk_grid_new();
	GError *error = NULL;
	GtkMediaStream *media_stream = gtk_media_file_new();
	GtkWidget *audio_controls = gtk_media_controls_new(media_stream);

        const GActionEntry entries[] = {
                {"browse-machine-to-play", activate_browse_to_play}
        };
        struct browse_machine_to_play_data *bm = (struct browse_machine_to_play_data *)malloc(sizeof(struct browse_machine_to_play_data));
        if (bm == NULL) {
                printf("Error allocating memory.\n");
                exit(EXIT_FAILURE);
        }
        bm->fd = file_dialog;
	bm->mf = media_stream;
	bm->mc = audio_controls;
        bm->label = current_song;
	g_action_map_add_action_entries(G_ACTION_MAP(app), entries, 1, bm);

	GMenu *menubar = g_menu_new();
        GMenuItem *browse_menu_item = g_menu_item_new("Browse machine for song to play", "app.browse-machine-to-play");
        g_menu_append_item(menubar, browse_menu_item);
        browse_machine_button = gtk_menu_button_new();
        gtk_menu_button_set_menu_model(browse_machine_button, menubar);   
        gtk_menu_button_set_label(browse_machine_button, "Select song to play");
	
	playlists_menu_button = gtk_menu_button_new();
	gtk_menu_button_set_label(playlists_menu_button, "Playlists");
	const GActionEntry entries1[] = {
                {"create-new-playlist", create_new_playlist}
        };
	struct create_new_playlist_data *cpd = (struct create_new_playlist_data *)malloc(sizeof(struct create_new_playlist_data));
	if (cpd == NULL) {
		printf("Error allocating memory for create_new_playlist_data.\n");
		exit(EXIT_FAILURE);
	}
	save_playlist_button = gtk_button_new();
	playlist_textview = gtk_text_view_new();
	cpd->main_grid = grid;
	cpd->subgrid = create_playlist_grid;
	playlist_name_input = gtk_text_new();
        playlist_name_prompt = gtk_label_new("Playlist name: ");
	cpd->playlist_textview = playlist_textview;
	struct add_song_to_playlist_data *asd = (struct add_song_to_playlist_data *)malloc(sizeof(struct add_song_to_playlist_data));
        if (asd == NULL) {
                printf("Error allcoating memory for add song data struct.\n");
                exit(EXIT_FAILURE);
        }
        asd->playlist_textview = playlist_textview;
        asd->file_dialog = file_dialog;
	playlist_add_song_button = gtk_button_new();
        gtk_button_set_label(playlist_add_song_button, "Add song");
        g_signal_connect(playlist_add_song_button, "clicked", add_song_to_playlist_cb, asd);
        cpd->playlist_add_song_button = playlist_add_song_button;
        cpd->save_playlist_button = save_playlist_button;
        cpd->playlist_name_input = playlist_name_input;
        cpd->playlist_name_prompt = playlist_name_prompt;
	gtk_grid_attach(cpd->subgrid,
                        cpd->playlist_name_prompt,
                        0, 0, 1, 1);
        gtk_grid_attach(cpd->subgrid, 
                        cpd->playlist_name_input,
                        1, 0, 1, 1);
                                    
        gtk_grid_attach(cpd->subgrid,  
                        cpd->playlist_add_song_button,
                        2, 0, 1, 1);
        gtk_grid_attach(cpd->subgrid,
                        cpd->playlist_textview,
                        0, 1, 3, 3);
        gtk_grid_attach(cpd->subgrid,
                        cpd->save_playlist_button,
                        1, 4, 1, 1);
	gtk_grid_attach(cpd->main_grid, cpd->subgrid, 1, 3, 1, 1);
	gtk_widget_hide(cpd->subgrid);

        gtk_button_set_label(save_playlist_button, "Save Playlist");
        g_signal_connect(save_playlist_button, "clicked", save_playlist_cb, cpd);

	g_action_map_add_action_entries(G_ACTION_MAP(app), entries1, 1, cpd);
	GMenu *playlists_menu = g_menu_new();
	GMenuItem *playlists_menu_item1 = g_menu_item_new("Create new playlist",
		       					  "app.create-new-playlist");
	g_menu_append_item(playlists_menu, playlists_menu_item1);

	struct play_playlist_data *ppd = (struct play_playlist_data *)malloc(sizeof(struct play_playlist_data));
	if (ppd == NULL) {
		printf("Error allcoating memory for play playlist data structure.\n");
		exit(EXIT_FAILURE);
	}
	cpd->ppd = ppd;
	struct playlist_node *playlist = (struct playlist_node *)malloc(sizeof(struct playlist_node));
	if (playlist == NULL) {
		printf("Error allocating memory for playlist.\n");
		exit(EXIT_FAILURE);
	}
	playlist->succ = 0;
	playlist->prev = 0;
	playlist->song = 0;
	
	ppd->cpd = cpd;
	ppd->current_song = current_song;	
	ppd->media_file = media_stream;
	ppd->media_controls = audio_controls;
	ppd->main_grid = grid;
	ppd->subgrid = play_playlist_grid;
	ppd->file_dialog = file_dialog;
	gtk_widget_hide(play_playlist_grid);
	ppd->playlist = playlist;
	play_playlist_textview = gtk_text_view_new();
	select_playlist_to_play = gtk_button_new_with_label("Select playlist");
	play_playlist_button = gtk_button_new_with_label("Play playlist");
	playlist_name = gtk_label_new("test label");
	ppd->playlist_name = playlist_name;
	ppd->play_playlist_textview = play_playlist_textview;
        ppd->select_playlist_to_play = select_playlist_to_play;
        ppd->play_playlist_button = play_playlist_button;
	g_signal_connect(select_playlist_to_play, "clicked", select_playlist_cb, ppd);
	g_signal_connect(play_playlist_button, "clicked", play_playlist_cb, ppd);
	const GActionEntry entries2[] = {
		{"play-playlist", play_playlist}
	};
	g_action_map_add_action_entries(G_ACTION_MAP(app), entries2, 1, ppd);
	GMenuItem *play_playlist_menu_item = g_menu_item_new("Play playlist", "app.play-playlist");
	g_menu_append_item(playlists_menu, play_playlist_menu_item);
        gtk_grid_attach(ppd->main_grid, ppd->subgrid, 1, 3, 1, 1);
        gtk_grid_attach(ppd->subgrid,
                        ppd->playlist_name,
                        0, 0, 1, 1);
        gtk_grid_attach(ppd->subgrid,
                        ppd->select_playlist_to_play,
                        1, 0, 1, 1);
        gtk_grid_attach(ppd->subgrid,
                        ppd->play_playlist_textview,
                        0, 1, 2, 2);
        gtk_grid_attach(ppd->subgrid,
                        ppd->play_playlist_button,
                        1, 3, 1, 1);
	gtk_widget_hide(ppd->subgrid);
	
	
	g_signal_connect(prev_button, "clicked", prev_song_cb, ppd);
	g_signal_connect(next_button, "clicked", next_song_cb, ppd);

	struct edit_playlist_data *epd = (struct edit_playlist_data *)malloc(sizeof(struct edit_playlist_data));
	if (epd == NULL) {
		printf("Error allocating memory for edit playlist data. %s.\n", strerror(errno));
		exit(EXIT_FAILURE);
	}
	ppd->epd = epd;
	cpd->epd = epd;
	epd->main_grid = grid;
	epd->subgrid = gtk_grid_new();
	epd->playlist_name_prompt = gtk_label_new("Playlist name: ");
        epd->playlist_name_input = gtk_text_new();
        epd->playlist_textview = gtk_text_view_new();
	epd->select_playlist = gtk_button_new_with_label("Select Playlist to Edit");
        epd->add_song_button = gtk_button_new_with_label("Add song to Playlist");
        epd->save_playlist_button = gtk_button_new_with_label("Save Playlist");
	epd->ppd = ppd;
	epd->cpd = cpd;
	struct add_song_to_playlist_data *asp = (struct add_song_to_playlist_data *)malloc(sizeof(struct add_song_to_playlist_data));
	if (asp == NULL) {
		printf("Error allocating memory for adding song to playlist data. %s.\n", strerror(errno));
		exit(EXIT_FAILURE);
	}
	asp->playlist_textview = epd->playlist_textview;
	asp->file_dialog = file_dialog;
	g_signal_connect(epd->add_song_button, "clicked", add_song_to_playlist_cb2, asp); 
	g_signal_connect(epd->save_playlist_button, "clicked", save_edited_playlist_cb, epd);
	g_signal_connect(epd->select_playlist, "clicked", select_playlist_cb2, epd);

	gtk_grid_attach(epd->subgrid, epd->playlist_name_prompt, 0, 0, 1, 1);
	gtk_grid_attach(epd->subgrid, epd->playlist_name_input, 1, 0, 2, 1);
	gtk_grid_attach(epd->subgrid, epd->add_song_button, 0, 4, 1, 1);
	gtk_grid_attach(epd->subgrid, epd->select_playlist, 3, 0, 1, 1);
	gtk_grid_attach(epd->subgrid, epd->playlist_textview, 0, 1, 3, 3);
	gtk_grid_attach(epd->subgrid, epd->save_playlist_button, 2, 4, 1, 1);
	gtk_grid_attach(epd->main_grid, epd->subgrid, 1, 3, 1, 1);
	gtk_widget_hide(epd->subgrid);

	const GActionEntry entries3[] = {
                {"edit-playlist", edit_playlist}
        };
        g_action_map_add_action_entries(G_ACTION_MAP(app), entries3, 1, epd);
	GMenuItem *edit_playlist_menu_item = g_menu_item_new("Edit playlist", "app.edit-playlist");
        g_menu_append_item(playlists_menu, edit_playlist_menu_item);

	gtk_menu_button_set_menu_model(playlists_menu_button, playlists_menu);



	window = gtk_application_window_new (app);
	gtk_window_set_title (GTK_WINDOW (window), "Window");
	gtk_window_set_default_size (GTK_WINDOW (window), 600, 400);

	gtk_window_set_child(GTK_WINDOW(window), grid);
	gtk_grid_set_column_spacing(grid, 0);
 	gtk_box_append(menu_box, browse_machine_button);
	gtk_box_append(menu_box, playlists_menu_button);
	gtk_box_append(playlist_track_box, audio_controls);	
	gtk_grid_attach(GTK_GRID(grid), playlist_track_box, 0, 2, 1, 1);
	gtk_grid_attach(GTK_GRID(grid), current_song, 0, 1, 1, 1);
	gtk_grid_attach(GTK_GRID(grid), menu_box, 0, 0, 3, 1);
	gtk_grid_attach(GTK_GRID(grid), play_playlist_grid, 1, 3, 1, 1);
	gtk_window_present (GTK_WINDOW (window));
}

int
main (int    argc,
      char **argv)
{
  char *home_dir = getenv("HOME");
  int home_dir_len = strlen(home_dir);
  char *home_suffix = "/Music/music_player_playlists";
  int home_suffix_len = strlen(home_suffix);
  char *playlists_dir = (char *)malloc(home_dir_len + home_suffix_len + 1);
  if (playlists_dir == NULL) {
  	printf("Error allocating memory to hold playlists directory absolute path name.%s.\n", strerror(errno));
  	exit(EXIT_FAILURE);
  }
  memcpy(playlists_dir, home_dir, home_dir_len);
  memcpy(&(playlists_dir[home_dir_len]), home_suffix, home_suffix_len);
  playlists_dir[home_dir_len + home_suffix_len] = 0;
  if ( chdir(playlists_dir) == -1) {
	printf("Error changing directory to ~/Music/music_player_playlists. %s.\n",
	strerror(errno));
	exit(EXIT_FAILURE);
  }
  free(playlists_dir);

  GtkApplication *app;
  int status;

  app = gtk_application_new ("org.gtk.example", G_APPLICATION_DEFAULT_FLAGS);
  g_signal_connect (app, "activate", G_CALLBACK (activate), NULL);

  status = g_application_run (G_APPLICATION (app), argc, argv);
  g_object_unref (app);

  return status;
}

