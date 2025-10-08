# Questions - TME 3 : Threads

Instructions : copiez vos réponses dans ce fichier (sous la question correspondante). A la fin de la séance, commitez vos réponses.

## Question 1.

```
cd build-release && ./TME3 ../WarAndPeace.txt freqstd && ./TME3 ../WarAndPeace.txt freqstdf && ./TME3 ../WarAndPeace.txt freq && check.sh *.freq


traces pour les 3 modes, invocation a check qui ne rapporte pas d'erreur
```

## Question 2.

start vaut 0

end vaut fileSize

Code des lambdas :
```
[&](const std::string& word) {
                        total_words++;
                        um[word]++;
                });

 pr::processRange(filename, 0, file_size, [&](const std::string& word) {
                        total_words++;
                        hm.incrementFrequency(word);
                });
```

Accès identifiés : total words et um (unsorted map) ou total words et hm (le hashmap)

## Question 3.

Continuez de compléter ce fichier avec vos traces et réponses.

SEGFAULT à cause de l'accès concurrent à um (unsorted map) concurrent. Idem le compte des mots est partagé...

Ca ne suffit pas de juste mettre atomic sur total_words, il faut aussi protéger l'accès à um (ou hm) avec un mutex.

Total runtime (wall clock) : 451 ms en 4 threads avec mutex vs 429 ms (mono)
Total runtime (wall clock) : 561 ms pr 100 threads (la création a un coût...)

Preparing to parse ../WarAndPeace.txt (mode=mt_hashes N=8), containing 3235342 bytes
Total runtime (wall clock) : 181 ms