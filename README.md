Tema 4 - Protocoale de Comunicatii (PCOM) 2025

Descriere generala

Aceasta tema presupune implementarea unui client HTTP complet functional in limbajul C, care interactioneaza cu un server REST ce simuleaza un sistem de biblioteca de filme. Aplicatia implementeaza atat functionalitati pentru administratori (crearea si stergerea de utilizatori), cat si pentru utilizatori obisnuiti (accesarea si gestionarea de filme si colectii). Comunicarea se realizeaza exclusiv prin socketuri TCP, iar datele sunt transmise in format JSON, utilizand biblioteca externa Parson pentru parsarea si serializarea acestora.

Functionalitatea este impartita in module, fiecare raspunzand pentru o componenta specifica a protocolului sau aplicatiei. Interfata este de tip CLI (command-line interface), utilizatorul interactionand cu aplicatia prin intermediul comenzilor text introduse la consola.

Structura proiectului

* **client.c** – contine implementarea tuturor comenzilor, gestionarea sesiunii, interactiunea cu utilizatorul si interpretarea raspunsurilor serverului;
* **requests.c/.h** – include functii pentru construirea cererilor HTTP de tip GET, POST, PUT, DELETE cu anteturi, cookie-uri si corp JSON;
* **helper.c/.h** – abstractizeaza operatiile de socket (deschidere conexiune, trimitere mesaj, receptie mesaj, inchidere conexiune);
* **parson.c/.h** – biblioteca externa utilizata pentru manipularea obiectelor JSON (parsare si construire de mesaje JSON);

Functionalitati implementate

Autentificare administrator

* `login_admin`: Permite autentificarea ca administrator. Se trimite un POST catre ruta `/api/v1/tema/admin/login` cu username si parola in corpul cererii. In urma autentificarii reusite, se extrage cookie-ul de sesiune din antetul "Set-Cookie".

* `logout_admin`: Trimite un GET catre ruta `/api/v1/tema/admin/logout` si invalideaza sesiunea administratorului, curatand cookie-ul stocat.

Gestionarea utilizatorilor (doar pentru admin)

* `add_user`: Trimite o cerere POST la ruta `/api/v1/tema/admin/users` cu datele de autentificare ale utilizatorului (username, parola). Se foloseste cookie-ul administratorului pentru autorizare.

* `get_users`: Trimite o cerere GET la aceeasi ruta. Se afiseaza toti utilizatorii existenti impreuna cu id-ul si parola lor.

* `delete_user`: Trimite o cerere DELETE catre `/api/v1/tema/admin/users/<username>` pentru a elimina utilizatorul specificat. Este nevoie de sesiunea administratorului validata prin cookie.

Autentificare utilizator

* `login`: Utilizatorul se autentifica folosind username, parola si numele adminului care l-a creat. Daca autentificarea reuseste, se retine cookie-ul de sesiune.

* `get_access`: Dupa login, se trimite o cerere GET la `/api/v1/tema/library/access` pentru a obtine token-ul JWT. Acesta este folosit in toate cererile ulterioare de tip utilizator.

Gestionare filme

* `get_movies`: Afiseaza toate filmele disponibile. Cererea GET include cookie-ul de sesiune si tokenul JWT in antet.

* `get_movie`: Afiseaza detaliile unui film dupa ID, folosind ruta `/api/v1/tema/library/movies/<id>`.

* `add_movie`: Creeaza un film nou printr-o cerere POST care contine titlul, anul, descrierea si ratingul. Campurile sunt citite de la tastatura si codificate ca JSON.

* `update_movie`: Modifica un film existent printr-o cerere PUT la ruta `/movies/<id>`. Se trimit noile campuri in corpul JSON.

* `delete_movie`: Sterge un film dupa ID. Se foloseste o cerere DELETE autentificata.

Gestionare colectii de filme

* `add_collection`: Creeaza o colectie noua pe baza unui titlu si a unei liste de ID-uri de filme. Se trimite un POST pentru colectie, urmat de cereri POST separate pentru fiecare film adaugat in colectie. Se gestioneaza si cazurile in care adaugarea unui film poate esua.

* `get_collections`: Returneaza toate colectiile existente ale utilizatorului autentificat. Raspunsul este afisat cu ID-ul si titlul fiecarei colectii.

* `get_collection`: Returneaza detalii despre o colectie specifica, inclusiv titlul, autorul si lista de filme incluse in colectie.

* `delete_collection`: Trimite o cerere DELETE catre `/collections/<id>` si sterge colectia daca utilizatorul este proprietar.

* `add_movie_to_collection`: Permite adaugarea ulterioara a unui film in colectie. Se trimite un POST catre endpointul corespunzator si se verifica succesul adaugarii.

* `delete_movie_from_collection`: Elimina un film dintr-o colectie existenta printr-o cerere DELETE cu ambele ID-uri (colectie si film).

Implementare tehnica

Aplicatia este scrisa in C si foloseste direct socketuri POSIX pentru comunicatia cu serverul. Toate cererile HTTP sunt compuse manual, respectand standardul HTTP/1.1. Cererile sunt transmise cu `send()` si raspunsurile sunt preluate prin `recv()`, implementand un sistem robust care asigura receptionarea completa a mesajelor indiferent de dimensiunea lor.

Sunt tratate toate erorile returnate de server (401 Unauthorized, 404 Not Found, 403 Forbidden etc.) si sunt afisate mesaje clare in consola pentru utilizator. Aplicatia retine atat cookie-urile de sesiune, cat si tokenul JWT, pe durata unei sesiuni active, si le ataseaza corespunzator la fiecare cerere.

Inputul este procesat cu grija, fiecare comanda fiind asociata cu o functie separata, iar argumentele sunt preluate de la tastatura cu validare minima. Alocarile dinamice sunt curatate corespunzator la fiecare pas pentru a preveni memory leaks.

Comenzile pot fi introduse de la tastatura, iar aplicatia raspunde interactiv. Exista o bucla principala care citeste comenzile pana la `exit`.
