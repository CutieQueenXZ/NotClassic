# NotClassic -_-


A modified Classicube-based client focused on experimentation, QoL features,
and client-side behavior.

This project exists because curiosity > sanity.

## Features
- Client-side commands
- Teleport utilities
- UI & environment tweaks
- Client-only hacks (no server modification)
- Python-powered chat & block streaming

## Disclaimer
This client is for **educational and experimental purposes ONLY**.  
Some servers may restrict or disallow modified clients.  
Use at your own risk. Don’t be that guy.

## And i wanted to add somethin well...
"STOOOPID" - Root's response, just bored. :p

## Added / Changed Commands

### Building
- **Cuboid** – Safe, but may trigger "you can’t build that far away"
- **OCuboid** – Old-school cuboid, client-only, heavy on weak devices
- **SCuboid** – Cuboid without teleporting
- **LCuboid** – No distance limit, but slow as hell
- **DCuboid** – Dev-only, unfinished (don’t ask)

### Fun / Chat
- **rickroll** – Lyrics only, certified troll
- **n8ball** – Gen Z responses :D
- **lolcat** – Chat colorizer (`/lolcat` for auto)
- **uwu** – UwU translator  
  ⚠ Auto mode breaks `/client` → `/cwient`
- **leet** – Not fully cooked yet

### Streaming (Python required)
- **stream** – Live chat streaming  
  Port: `45678`
- **bstream** – Block streaming (experimental)  
  Port: `45679`
- **badapple** – ASCII Bad Apple  
  ⚠ Do NOT run on MCGalaxy (you *will* get kicked)

### AI / External
- **gemini** – Gemini API via Python  
  Port: `45700` (can freeze sometimes)
- **gpt** – ChatGPT API via Python  
  Port: `45701`

### Utility
- **antiafk** – Camera left/right movement
- **autoclick** –  
  `/client autoclick left|right|off [delay]`
- **ntp** – Player teleport (may error on partial names)
- **fullbright** – Environment-based fullbright

---

## Platform Notes
- **Desktop (Windows/Linux):** Supported
- **Android:** Experimental (bc not tested)
  Python features may require **Termux + port forwarding**
- **Root not strictly required**, but life is easier with it

---

## License
MIT
