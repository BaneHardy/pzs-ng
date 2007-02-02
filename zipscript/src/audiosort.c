#include <string.h>
#include <errno.h>

#ifndef HAVE_STRLCPY
# include "strl/strl.h"
#endif

#include "../conf/zsconfig.h"
#include "zsconfig.defaults.h"

#include "objects.h"
#include "zsfunctions.h"
#include "audiosort.h"
#include "multimedia.h"

void audioSortDir(char *targetDir)
{
	int cnt = 0;
	char link_source[PATH_MAX], link_target[PATH_MAX], *file_target;
	struct audio info;
	DIR *ourDir; 
	

	if (*targetDir != '/')
	{
		d_log("audioSortDir: not an absolute path. (%s)\n", targetDir);
		return;
	}

/* Look at something like that to verify that a release is complete?
 * (And tell user to run rescan if not)
 * char *sfv_data = NULL; 
 *
 * sfv_data = ng_realloc2(sfv_data, n, 1, 1, 1);
 * sprintf(sfv_data, storage "/%s/sfvdata", targetDir);
 * 
 * readsfv(sfv_data, &g.v, 0);
 */
	
	cnt = extractDirname(link_target, targetDir);
	strlcpy(link_source, targetDir, PATH_MAX);
	
	chdir(targetDir);
	if ((ourDir = opendir(targetDir)) == NULL) {
		printf("Error: Failed to open dir \"%s\" : %s\n", targetDir, strerror(errno));
		return;
	}
	file_target = findfileext(ourDir, ".mp3");

	get_mpeg_audio_info(file_target, &info);
	audioSort(&info, link_source, link_target);

	closedir(ourDir);
}

void audioSort(struct audio *info, char *link_source, char *link_target)
{
#if ( audio_genre_sort == TRUE ) || (audio_artist_sort == TRUE) || (audio_year_sort == TRUE) || (audio_group_sort == TRUE) || (audio_language_sort == TRUE)
	char *temp_p = NULL;
	char *temp_q = NULL;
	char temp_nam[NAME_MAX];
	int n = 0;
#if (audio_language_sort == TRUE)
	char language[3];
#endif
#else
	d_log("audioSort: No audio_*_sort is set to TRUE - skipping sorting of mp3\n");
	(void)info; (void)link_source; (void)link_target;
#endif

if (subcomp(link_target, NULL)) {
        int i, pos;
        pos = 0;
        for (i = strlen(link_source); i > 0; i--) {
                if (link_source[i] == '/')  {  pos = i; break; }
        }
        char sourceDir[PATH_MAX];
        strcpy(sourceDir, link_source);
        strlcpy(link_source, sourceDir, pos + 1);
        extractDirname(link_target, link_source);
}


#if ( audio_genre_sort == TRUE )
	d_log("audioSort:   Sorting mp3 by genre (%s)\n", info->id3_genre);
	createlink(audio_genre_path, info->id3_genre, link_source, link_target);
#endif
#if ( audio_artist_sort == TRUE )
	d_log("audioSort:   Sorting mp3 by artist\n");
	if (*info->id3_artist) {
		d_log("audioSort:     - artist: %s\n", info->id3_artist);
		if (!strncasecmp(link_target, "VA", 2) && (link_target[2] == '-' || link_target[2] == '_'))
			memcpy(info->id3_artist, "VA", 3);
		if (memcmp(info->id3_artist, "VA", 3)) {
#if ( audio_artist_nosub == FALSE )
			temp_p = ng_realloc(temp_p, 2, 1, 0, NULL, 1);
			snprintf(temp_p, 2, "%c", toupper(*info->id3_artist));
			createlink(audio_artist_path, temp_p, link_source, link_target);
#else
			temp_p = ng_realloc(temp_p, strlen(info->id3_artist)+1, 1, 0, NULL, 1);
			snprintf(temp_p, strlen(info->id3_artist)+1, "%s", info->id3_artist);
			temp_p = check_nocase_linkname(audio_artist_path, temp_p);
			space_to_dot(temp_p);
			createlink(audio_artist_path, temp_p, link_source, link_target);
#endif
			ng_free(temp_p);
		} else
			createlink(audio_artist_path, "VA", link_source, link_target);
	}
#endif
#if ( audio_year_sort == TRUE )
	d_log("audioSort:   Sorting mp3 by year (%s)\n", info->id3_year);
	if (*info->id3_year != 0)
		createlink(audio_year_path, info->id3_year, link_source, link_target);
#endif
#if ( audio_group_sort == TRUE )
	d_log("audioSort:   Sorting mp3 by group\n");
	temp_p = remove_pattern(link_target, "*-", RP_LONG_LEFT);
	temp_p = remove_pattern(temp_p, "_", RP_SHORT_LEFT);
	strncpy(temp_nam, temp_p, sizeof(temp_nam));
	n = (int)strlen(temp_p);
	if (n > 0 && n < 15) {
		if (n > 4) {
			temp_q = temp_nam + n - 4;
			if (!strncasecmp(temp_q, "_INT", 4)) {
				d_log("audioSort:   - Internal release detected\n");
				*temp_q = '\0';
			}
		}
		d_log("audioSort:   - Valid groupname found: %s (%i)\n", temp_nam, n);
		temp_p = check_nocase_linkname(audio_group_path, temp_p);
		d_log("audioSort:   - Valid groupname found: %s (%i)\n", temp_nam, n);
		createlink(audio_group_path, temp_nam, link_source, link_target);
	}
#endif
#if ( audio_language_sort == TRUE )
	d_log("audioSort:   Sorting mp3 by language\n");
	language[0] = '\0';
	n = (int)strlen(link_target);
	while ( n > 3) {
		if (link_target[n] == '-') {
			if ((unsigned char)link_target[n-3] == '-' && (unsigned char)link_target[n-2] >= 'A' && (unsigned char)link_target[n-2] <= 'Z' && (unsigned char)link_target[n-1] >= 'A' && (unsigned char)link_target[n-1] <= 'Z') {
				language[0] = link_target[n-2];
				language[1] = link_target[n-1];
				language[2] = '\0';
				temp_p = language;
				break;
			}
		}
		n--;
	}
	if (language[0] != '\0' && !strcomp(audio_ignored_languages, language)) {
		d_log("audioSort:   - Valid language found: %s\n", language);
		createlink(audio_language_path, temp_p, link_source, link_target);
	} else
		d_log("audioSort:   - No valid language found.\n");

#endif
}

