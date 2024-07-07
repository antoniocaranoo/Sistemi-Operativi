# SISTEMI-OPERATIVI

Progetto universitario di 'Sistemi Operativi'.
I linguaggi coinvolto è C.
Ci tengo a specificare che tale linguaggio non è mai stato fortemente trattato nei corsi univeritari, 
sono state trattate solo le basi, dunque c'è stata data una base di partenza
alla quale dovevamo aggiungere una di questa estensioni (nel mio progetto ce ne sono due: numero 1 e numero 4).


Proposte di estensione
1- Nelle shell dei sistemi Unix-like è possibile appendere il carattere & alla fine di una riga di comando per indicare che il comando va eseguito in background: ossia, la shell non deve bloccarsi in attesa della terminazione del comando, ma deve proseguire in parallelo al comando e visualizzare subito il prompt successivo. Implementate in dsh l'esecuzione di un comando in background attraverso il carattere &.

2- Nelle console dei sistemi Unix-like la combinazione di tasti CTRL-C genera il segnale SIGINT, che interrompe l'esecuzione di un processo. Quando premiamo CTRL-C, una shell come zsh o bash consegna il segnale al comando correntemente in esecuzione, interrompendolo, mentre se non c'è un comando in esecuzione la shell cancella il contenuto della riga di comando corrente e visualizza un nuovo prompt. Se invece premiamo CTRL-C in dsh usciamo da dsh. Implementare in dsh la gestione corretta del segnale SIGINT, in maniera che quando premiamo CTRL-C dsh non termini ma faccia terminare il comando corrente.

3- L'unica variabile della shell dsh è la variabile PATH, che può essere impostata e letta con il comando setpath. Si vuole far si che in dsh sia possibile definire variabili arbitrarie con nome qualsiasi utilizzando un comando interno

  set VAR buongiorno

e che sia possibile ottenere il valore della variabile utilizzando il carattere $ seguito dal nome della variabile; ad esempio il comando

  echo $VAR

deve stampare "buongiorno" a schermo. Eliminate il comando setpath, e fate si che sia possibile impostare la variabile PATH utilizzando set, e leggerne il contenuto utilizzando $.

4- Le shell come zsh e bash memorizzano la "storia" delle righe di comando che sono state inserite dall'utente: premendo ripetutamente  il tasto "freccia in alto" dopo il prompt viene inserito il testo dell'ultimo, penultimo... comando inserito dall'utente. Implementate questa funzionalità con una profondità di storia di 10 comandi (ossia, dsh deve memorizzare solo gli ultimi 10 comandi inseriti dall'utente).
