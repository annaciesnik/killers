
# Oznaczenia
W sprawozdaniu (niekoniecznie w kodzie) używane są następujące oznaczenia:

- *N*: liczba procesów firm oferujących zabójców
- *Z*: liczba zabójców w każdej z firm
- *P*: liczba wszystkich procesów
- *K*: liczba procesów klientów
- *S*: średni czas, jaki klienci oczekują między kolejnymi zleceniami
- *T*: średni czas wykonania zlecenia

# Założenia
Kanały komunikacyjne między procesami są typu FIFO i zapewniają niezawodną komunikację.
Środowisko w pełni asynchroniczne.
Czas działania, czas przeysłania kominikatów - brak założeń.

## Wymagania techniczne
OpenMPI 1.10 z opcją MPI_THREAD_MULTIPLE (zapewnia, że wywołania OpenMPI są thread-safe).

# Typy procesów

Procesy o identyfikatorze (rank) od 0 do N-1 to procesy reprezentujące firmy (oferujące usługi zabójców).
Pozostałe procesy (od N do P-1) to klienci.
Suma wszystkich procesów P jest zdefiniowana następująco:
  P = N + K

# Implementacja

## Klient

*K1*: Każdy proces klienta powinien zgłosić maksymalnie jedno zlecenie (aż do czasu końca jego realizacji). Potem może zgłosić następne.
  *Notes 1*: Wartość zmiennej "state" oznacza zarówno stan procesu klienta, jak i jego aktualnego zlecenia. Wyróżniamy następujące wartości zmiennej "state":
  - *WAITING*: Klient nie ma aktualnie zlecenia dla zabójcy;
  - *NOQUEUE*: Klient ma zlecenie do realizacji - informuje o tym usługodawców. Ponadto nie ma jeszcze żadnego wyznaczonego miejsca w kolejce.
  - *QUEUE*: Klient oczekuje na przyjęcie zlecenia do realizacji i wyznaczenie miejsca w kolejkach przez poszczególne firmy.
  - *INPROGRESS*: Zlecenia klienta jest w trakcie realizacji; Po otrzymaniu wiadomości typu *TAG_REQUEST* ze statusem *Q_DONE*, klient rozsyła swoją recenzję (w postaci oceny wykonanej usługi do wszystkich klientów).


*K2*: Klient powinien oczekiwać w stanie *WAITING* losowy okres czasu zanim zacznie ubigać się o usługę zabójcy. Po upływie tego czasu przechodzi do stanu *NOQUEUE*.

*K3*: Klient w stanie *NOQUEUE* powinien wysłać wiadomość typu *TAG_REQUEST* do wszystkich firm (aby sprawdzić dostępne miejsca w kolejkach), a następnie przejść do kolejnego stanu *QUEUE*.
  *Notes 1*: Liczba firm (N) jest z góry znana klientowi, więc znane są mu także numery procesów, do których powinien wysłać zlecenie.
  *Notes 2*: Potwierdzenie odbioru wiadomości typu *TAG_REQUEST* przez wszystkie firmy nie jest konieczne do kontynuowania przetwarzania zlecenia przez klienta.

*K4*: Po wykonaniu kroku *K3*, proces klienta powinien oczekiwać na przychodzące wiadomości typu *TAG_UPDATE*.

*K5*: W czasie oczekiwania na wiadomości typu *TAG_UPDATE* klient powinien również aktualizować informację na temat wystawionych recenzji.
  *Notes 1*: Nowe recenzje razem z aktualnym miejscem w kolejce mogą wpłynąć na zmianę preferencji klienta i rezygnację z kolejki u innych firm. W przypadku rezygnacji z oferty (miejsca w kolejce), klient powinien wysłać wiadomość typu *TAG_CANCEL* i oznaczyć odpowiednie miejsce w tablicy *queues* jako *Q_CANCELLED*.

*K5*: Po odebraniu wiadomości typu *TAG_UPDATE* z wartością nieujemną, klient powinien zapisać wyznaczone miejsce w kolejce n-tej firmy w tablicy "queues" na n-tej pozycji.
  *Notes 1*: Wyznaczone miejsce w kolejce n-tej firmy jest przesłane w wiadomości typu *TAG_UPDATE* jako wartość nieujemna. Wartości ujemne są zarezerwowane na inne komunikaty specjalnego przeznaczenia (np. *Q_IN_PROGRESS*, *Q_DONE* itd.);

*K6* Przejście klienta ze stanu *QUEUE* do następnego *INPROGRESS* powinno nastąpić po odebrania pierwszej wiadomości typu *TAG_UPDATE* ze statusem *Q_IN_PROGRESS*;
  *K6.1* W odpowiedzi na pierwszy komunikat *Q_IN_PROGRESS*, klient powinien wysłać wiadomość typu *ACK_OK* do tej firmy, a do pozostałych *ACK_REJECT*.
  *K6.2* Niezależnie od otrzymanych wiadomości, klient powinien zrezygnować z miejsc w kolejkach pozostałych firm poprzez wysłanie wiadomości typu *TAG_CANCEL* do każdej z nich i oznaczeniu odpowiednich miejsc w tablicy *queues* jako *Q_CANCELLED*.

*K7* Klient powinien oczekiwać na pomyślne wykonanie zlecenia (w stanie *INPROGRESS* na wiadomość typu *TAG_UPDATE* ze statusem *Q_DONE*).
  *Notes 1* Można założyć, że otrzymamy tylko jedną taką wiadomość (od firmy, która podjęła się zlecenia jako pierwsza).
  Oznacza to, że nie ma konieczności sprawdzania czy ta wiadomość pochodzi z dobrego źródła.
  (FIXME FYI odważne założenine)

  *Notes 2* Przetworzenie wiadomości typu *TAG_REVIEW* może nastąpić za każdym razem, kiedy proces klienta oczekuje na wiadomości typu *TAG_UPDATE*. Oznacza to, że kiedy proces nie ubiega się o zabójcę, wiadomości oczekują w kanale komunikacyjnym.
  (FIXME - potencjalnie to może być problem, bo kolejka może się zapchać po dłuższym oczekiwaniu w stanie *WAITING* albo może znacząco opóźnić odczyt wiadomości od firm.)

  *K7.1* Klient powinien rozesłać ocenę wykonanej usługi do wszystkich klientów. (Działa to jak marketing szeptany.)
  *K7.2* Klient powinien posprzątać informację na temat zajmowanych kolejek z tego zlecenia i przejść do stanu zadumy *WAITING*.




### Deskryptor stanu klienta (tzn. stanu zlecenia danego klienta)

#### Values of customer's state
-->
typedef enum State
{
    WAITING,       /* A customer has no new task/job yet for a killer. */
    NOQUEUE,       /* A customer has a task/job, but it is not in any queue yet.
                    * In this state the customer sends a request to all companies. */
    QUEUE,         /* A customer has been waiting for infomration, that the task/job is in progress by one of these companies. In this state the customer considers new review ratings (if any). */
    INPROGRESS     /* The task/job is in progress. Awaiting for conformation that it is completed. In this state the customer sends notification to other customers about his review rating. */
} State;
<--

#### Assigned places in queues (one entry per each company).
-->
int queues[N]
<--

The queues[N] table contains exactly one entry per each company.
Each entry represents the current status of the assigned place in a company's queue.
A nonnegative value represents the assigned place in a queue.
For other negative values see Q_CANCELLED, Q_NO_UPDATE_RECEIVED and others.




## Firma
Do procesu firmy należy główny wątek menadżera oraz wątek agenta.

*N1* Główny wątek powinien oczekiwać w pętli na przychodzące wiadomości.

*N2* Wątek agenta powinien oczekiwać na wykonanie zlecenia przez któregokolwiek z zabójców danej firmy.
Notes: Każda firma ma przypisaną listę Z zabójców.

*N3* Wątek agenta powinien wysłać wiadomość *TAG_KILLER_READY* do głównego wątku firmy kiedy którykolwiek zabójca zakończył swoje zadanie.

*N4* Menadżer powinien dodać klienta do kolejki jeśli otrzymał wiadomość typu *TAG_REQUEST* od klienta.
  *N4.1* Menadżer powinien przydzielić zabójcę od razu jeśli kolejka była uprzednio pusta (tzn. jest aktualnie jeden klient oczekujący).

*N5* Menadżer powinien anulować rezerwację klienta, kiedy otrzymał wiadomość typu *TAG_CANCEL*.

*N6* Menadżer powinien przydzielić kolejne zlecenie zabójcy z puli oczekujących, jeśli otrzymał wiadomość *TAG_KILLER_READY* od agenta.

*N7* Przydzielanie zlecenia wyznaczonemu zabójcy:

*N7.1* Jeśli aktualnie rozważany zabójca ma nadal przypisanego klienta (wartość w polu <client> jest nieujemna), menadżer powinien:
 1) Wysłać wiadmość typu: *TAG_UPDATE* ze statusem *Q_DONE*.
 2) Wyczyścić informację o poprzednim kliencie (ustawić ) <client> na *NO_CLIENT*).
 3) Zaktualizować status zabójcy (ustawić pole <status> na *K_READY*).

 *N7.2* Jeśli jest jakiekolwiek oczekujące zlecenie w kolejce, menadżer powinien:
  1) Wysłać wiadomość typu *TAG_UPDATE* ze statusem *Q_IN_PROGRESS* do pierwszego klienta z listy oczekujących.
  2) Oczekiwać na wiadomość typu *TAG_ACK*.
  *N7.3* Jeśli klient potwierdził chęć wykonania zlecenia (wiadomość typu *TAG_ACK* ze statusem *ACK_OK*), menadżer powinien przypisać tego klienta w polu <client>, ustawić przewidywany czas zakończenia zlecenia i ustawić <status> zabójcy na K_BUSY.

  *N7.4* Jeśli klient odrzucił ofertę firmy (wiadomość typu *TAG_ACK* ze statusem *ACK_REJECT*), menadżer powinien pobrać kolejne zlecenie z kolejki do rozważenia (*N7.2*).


  *N8* Agent powinien sprawdzić jaki jest najkrótszy czas realizacji zlecenia dla zabójców ze zleceniem (<status> *K_BUSY*) i ustawić sobie czas oczekiwania.

  *N9* Agent powinien odpytać wszystkich zabójców z przydzielonym zleceniem i wysłać wiadomość do menadżera (TAG_KILLER_READY) z informacją który zabójca może podjąć się kolejnego zlecenia.



### Deskryptor firmy (company)
Kolejka *Queue* danej firmy może zawierać maksymalnie zlecenia od K klientów. Pozycja w tablicy powinna odpowiadać wyznaczonemu miejscu w kolejce. Natomiast q-ty element tablicy powinien zawierać ID procesu klienta. Aktualny rozmiar kolejki powinien być zapisany w zmiennej *queueLen*.
-->
/* The Queue[customers]
 * Queue[queuePos] is a client's process number */
int Queue[K];     

/* Length of the current queue with customers */
int queueLen;
<--

Lista zabójców jest zasobem współdzielonym pomiędzy głównym wątkiem w procesie firmy, a wątkiem agenta.
Dostęp do listy jest chroniony poprzez mutex *killersMute*.
Liczba zabójców *Z* ustalana przez użytkownika jest jednocześnie rozmiarem tablicy *Killers*. Jest ona jednakowa dla wszystkich firm.
-->
/* The list of killers of the current company. */
struct Killer Killers[Z];

/* Mutex to protect the list of 'Killers' */
pthread_mutex_t killersMutex;   
<--

Deskryptor zabójcy zawiera następujące informacje:
- *client* - Aktualnie obsługiwany klient (lub *NO_CLIENT*, jeśli brak).
- *status* - Aktualny status zabójcy (m.in. *K_READY* lub *K_BUSY*).
-


-->
/* Descriptor of a killer */
typedef struct Killer {
    int client;                /* The current customer, which is served */
    int status;                /* Status of the killer */
    Timespec timer;            /* Time related with the current task ?? (time to the end)?? */
} Killer;
<--

Rozróżniamy następujące statusy zabójcy:
- *K_READY* - Oczekiwanie na zlecenie.
- *K_BUSY* - Zabójca w trakcie zlecenia.
- *K_NOTIFICATION_SENT* - Zabójca wykonał zadanie i wysłał informację zwrotną. Oczekuje na dalszą decyzję ze strony głównego wątku firmy - manager'a.

Bezpośrednia kominikacją pomiędzy głównym wątkiem menadżera, a wątkiem agenta odbywa się za pomocą zmiennej warunkowej *wakeUpAgent*. Agent oczekuje na zakończenie zlecenia przez któregokolwiek ze swoich podopiecznych.
Jeśli główny wątek przydzieli kolejne zlecenie wolnemu zabójcy (lub takiemu, który akurat wrócił z misji), wówczas wysyła sygnał do agenta. Agent wówczas budzi się na chwilę, sprawdza jaki jest najkrótszy deklarowany czas realizacji zlecenia.
-->
conditional wakeUpAgent
<--

# Założenia
## Pojemność kanałów
Maksymanie w kanale naraz może być ... wiadomości..
##
- pojemność kanałów (ile wiadomości maksymalnie naraz może być w kanale),
[Q] Ile?
=======
[Odp] Liczba procesów klientów (per firemka), bo w najgorszym przypadku wszyscy klienci wysyłają:
 -> Jest to funkcja ilości zleceń. Jeśli firemka x wykona zlecenie i otrzyma fatalną punktację, to klienci mogą cyklicznie wysyłać
 Firemka na każde zlecenie może otrzymać request + cancel i te wiadomości mogą oczekiwać w kanale. Firemka z kolei może oczekiwać na wiadomość typu ACK.
 Jeśli klienci rozważają kolejne zlecenia, a firemka nadal oczekuje w stanie ACK, to kanał zostanie bardzo szybko zapchany.
 Przyjęcie ACK jest krytyczne ;)

 Maksymalna liczba zleceń (niekoniecznie liczba klientów) w na jednostkę czasu * 2. Gdzie w jednostce czasu ....

- oraz złożoność komunikacyjną i czasową
[Q] Jaka jest?
===============
-> Na jedno zlecenie (nie koniecznie na jednego klienta) złożoność komunikacyjna:
=>>
 N (dla wiadomości request) + N (na wiadomości cancel) + N*średnia liczba procesów w kolejce <= 2N + N*K;
 <<=

 średnia liczba procesów w kolejce = liczba klientów* ??
 czyli 2N + N*K

 złożoność czasowa:
 =>>
 Jednostka czasu - wysłanie i odebranie wiadomości między procesami.
 Złożoność czasowa wynosi 4 ;)
 Req+ update z info IN_PROGRESS;
 ACK + update z info DONE
 <<=


# Uruchomienie programu

make debug
mpirun -np P killers -c N -k Z [-s S -t T]
