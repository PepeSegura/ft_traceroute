#ifndef HELP_H
# define HELP_H

# define HELP "Usage: traceroute [OPTION...] HOST ...\n\
Print the route packets trace to network host.\n\
\n\
  -f, --first-hop=NUM        set initial hop distance, i.e., time-to-live\n\
  -m, --max-hop=NUM          set maximal hop count (default: 64)\n\
  -p, --port=PORT            use destination PORT port (default: 33434)\n\
  -q, --tries=NUM            send NUM probe packets per hop (default: 3)\n\
      --resolve-hostnames    resolve hostnames\n\
  -w, --wait=NUM             wait NUM seconds for response (default: 3)\n\
  -?, --help                 give this help list\n\
      --usage                give a short usage message\n\
\n\
Mandatory or optional arguments to long options are also mandatory or optional\n\
for any corresponding short options.\n\
\n\
Report bugs to <psegura-@student.42madrid.com>.\
"

#define USAGE "Usage: traceroute [-?] [-f NUM] [-m NUM] [-p PORT]\n\
            [-q NUM] [-w NUM] [--first-hop=NUM]\n\
            [--max-hop=NUM] [--port=PORT]\n\
            [--tries=NUM] [--resolve-hostnames] [--wait=NUM]\n\
            [--help] [--usage] HOST\n\
"

#endif
