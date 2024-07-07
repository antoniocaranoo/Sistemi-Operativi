README RSO
Antonio Carano 902447
Camilla Cantaluppi 894557

Compilazione del file con:
gcc dsh.c -lreadline -o dsh

In un'unica shell sono state implementate le estensioni numero 1 e numero 4.

Per l'estensione 1 è stato implementato un controllo per verificare che l'ultimo parametro inserito a riga di comando sia '&'. Se tale controllo ritorna vero, allora quel comando sarà eseguito in background, e la '&' verrà rimossa dalla lista dei parametri. In questo modo ci si assicurerà che il processo figlio andrà a svolgere le sue attività in background.
Di conseguenza la shell non si blocca e resta disponibile per ulteriori input, dato che il processo padre non aspetterà la terminazione del processo figlio.
Inoltre, per gestire i processi zombie in questo programma, possiamo fare uso della funzione "waitpid" per attendere la terminazione dei processi figli e rimuoverli dalla tabella dei processi del sistema operativo una volta terminati. Possiamo anche utilizzare un array, per tenere traccia dei processi figli che sono stati avviati in modalità background.
Ecco come abbiamo proceduto:
	1) Creazione un array per tenere traccia dei PID dei processi figli.
	2) Quando viene avviato un processo figlio, viene aggiunto il suo PID all'array.
	3) Quando un processo figlio termina, viene rimosso il PID dall'array e viene fatta la "waitpid" per rimuovere lo stato zombie.
	4) Ogni volta che viene eseguito un nuovo comando, viene controllato se ci sono processi zombie e in caso affermativo, questi vengono rimossi.

Per quanto riguarda l'estensione 4, la shell mantiene una cronologia degli ultimi 10 comandi inseriti. Il comando 'history' mostra questa lista, permettendo all'utente di rivedere i comandi eseguiti. Inoltre, l'utilizzo della libreria 'readline' abilita la navigazione attraverso la cronologia dei comandi usando il tasto "arrow up", facilitando la riusabilità dei comandi precedenti. Per implemetare 'history' viene usato un array, mentre lo scorrimento della history attraverso i tasti "arrow up" e "arrow down" è implementato tramite la libreria 'readline'.
Ciò ha portato alla modifica della funzione prompt precedentemente implementata dal professore durante le esercitazioni, dato che è stato necessario cambiare l'acquisizione di comandi da linea di comando. Questa era una delle possibili scelte, tra le quali c'erano l'utilizzo di 'fgetc' e la funzione 'getch' che erano anche esse opzioni valide ma di diversa natura.
Una volta usciti dalla shell, i comandi salvati nella history vengono dimenticati. Se si volesse implementare un'estensione che mantiene salvati i comandi anche dopo la chiusura della shell, sarebbe utile usare le seguenti funzioni: 'read_history(".dsh_history")' e 'write_history(".dsh_history")'.