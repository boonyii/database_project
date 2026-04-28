
#include "record.h"
#include "utils.h"
#include <stdio.h>
#include <string.h>

int parse_record(const char *line, Record *rec) {
    char datebuf[32];
    int team, pts, ast, reb, win;
    float fg, ft, fg3;

    // Try full 9-field parse
    int n = sscanf(line, "%31s %d %d %f %f %f %d %d %d",
                   datebuf, &team, &pts, &fg, &ft, &fg3,
                   &ast, &reb, &win);

    if (n == 9) {
        encode_date(datebuf, rec->date);
        rec->team_id = compress_team_id(team);
        rec->pts     = (uint8_t)pts;
        rec->fg_pct  = encode_pct(fg);
        rec->ft_pct  = encode_pct(ft);
        rec->fg3_pct = encode_pct(fg3);
        rec->ast     = (uint8_t)ast;
        rec->reb     = (uint8_t)reb;
        rec->win     = (uint8_t)win;
        return 1;
    }

    // Try 3-field preseason parse
    n = sscanf(line, "%31s %d %d", datebuf, &team, &win);
    if (n == 3) {
        encode_date(datebuf, rec->date);
        rec->team_id = compress_team_id(team);
        rec->pts     = 0;
        rec->fg_pct  = 0;
        rec->ft_pct  = 0;
        rec->fg3_pct = 0;
        rec->ast     = 0;
        rec->reb     = 0;
        rec->win     = 0;   // preseason games don't have result here
        return 1;
    }

    // Debugging: uncomment to see skipped lines
    // fprintf(stderr, "Skipped line: %s", line);

    return 0;
}

void print_record(const Record *rec) {
    char datebuf[32];
    decode_date(rec->date, datebuf, sizeof(datebuf));

    // Detect preseason (all stats zero)
    if (rec->pts == 0 && rec->ast == 0 && rec->reb == 0 &&
        rec->fg_pct == 0 && rec->ft_pct == 0 && rec->fg3_pct == 0) {
        printf("Date=%s  Team=%u  PRESEASON (no stats)\n",
               datebuf,
               expand_team_id(rec->team_id));
        return;
    }

    // Normal game
    printf("Date=%s  Team=%u  PTS=%u  FG%%=%.3f  FT%%=%.3f  FG3%%=%.3f  AST=%u  REB=%u  WIN=%u\n",
           datebuf,
           expand_team_id(rec->team_id),
           rec->pts,
           decode_pct(rec->fg_pct),
           decode_pct(rec->ft_pct),
           decode_pct(rec->fg3_pct),
           rec->ast,
           rec->reb,
           rec->win);
}
