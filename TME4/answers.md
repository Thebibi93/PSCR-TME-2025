# TME4 Answers

Tracer vos expériences et conclusions dans ce fichier.

Le contenu est indicatif, c'est simplement la copie rabotée d'une IA, utilisée pour tester une version de l'énoncé.
On a coupé ses réponses complètes (et souvent imprécises voire carrément fausses, deadlocks etc... en Oct 2025 les LLM ont encore beaucoup de mal sur ces questions, qui demandent du factuel et des mesures, et ont de fortes tendances à inventer).
Cependant on a laissé des indications en particulier des invocations de l'outil possibles, à adapter à votre code.

## Question 1: Baseline sequential

### Measurements (Release mode)

**Resize + pipe mode:**
```
./build/TME4 -m resize -i input_images -o output_images
Image resizer starting with input folder 'input_images/', output folder 'output_images/', mode 'resize', nthreads 4
Thread 130046506711360 (main): 2537 ms CPU
Total runtime (wall clock): 2598 ms
Memory usage: Resident: 45 MB, Peak: 103 MB
Total CPU time across all threads: 2537 ms

./build/TME4 -m pipe -i input_images -o output_images

Image resizer starting with input folder 'input_images/', output folder 'output_images/', mode 'pipe', nthreads 2
Thread 123540114429632 (treatImage): 7886 ms CPU
Thread 123540159686976 (main): 5 ms CPU
Total runtime (wall clock): 8058 ms
Memory usage: Resident: 45.2 MB, Peak: 135 MB
Total CPU time across all threads: 7891 ms
```



## Question 2: Steps identification

I/O-bound: find, load et save
CPU-bound: resize

parallelisable a priori ?

On peut charger les images en parallèle, et les sauver en parallèle (une tache ne dépend pas de l'autre).

## Question 3: BoundedBlockingQueue analysis

Il peut y avoir plusieurs producteurs et plusieurs consommateurs toutes les opérations sont synchronisées.

Le lambda capture this qui est un pointeur vers l'instance de la classe BoundedBlockingQueue. La condition_variable a besoin de capturer this pr pouvoir accéder aux membres. 
Le lambda est recommandé pour éviter de devoir écrire directement un while.

## Question 4: Pipe mode study

FILE_POISON sert à arrêter les threads consommateurs.

Order/invert :
On ne peut pas inverser l'ordre des étapes.
La 2 et 3 crée un deadlock si on inverse.
On doit push le poison après avoir peupler notre fileQueue 
sinon le thread finira avant de tout traiter.
Le join doit être fait après le push du poison sinon on ne 
pousse jamais le poison et le thread ne finit jamais.


## Question 5: Multi-thread pipe_mt

Implement pipe_mt mode with multiple worker threads.

For termination, ... poison pills...

Measurements:
- N=1: 
```
Image resizer starting with input folder 'input_images/', output folder 'output_images/', mode 'pipe_mt', nthreads 1
Thread 135738742957760 (treatImage): 2511 ms CPU
Thread 135738794547520 (main): 3 ms CPU
Total runtime (wall clock): 2577 ms
Memory usage: Resident: 57 MB, Peak: 115 MB
Total CPU time across all threads: 2514 ms
...
```
- N=2: 
```
./build/TME4 -m pipe_mt -n 2 -i input_images -o output_images
Image resizer starting with input folder 'input_images/', output folder 'output_images/', mode 'pipe_mt', nthreads 2
Thread 129028686390976 (treatImage): 3891 ms CPU
Thread 129028677998272 (treatImage): 4047 ms CPU
Thread 129028738591040 (main): 5 ms CPU
Total runtime (wall clock): 4156 ms
Memory usage: Resident: 97.8 MB, Peak: 251 MB
Total CPU time across all threads: 7943 ms
```
- N=4: 
```
./build/TME4 -m pipe_mt -n 4 -i input_images -o output_images
Image resizer starting with input folder 'input_images/', output folder 'output_images/', mode 'pipe_mt', nthreads 4
Thread 133669170747072 (treatImage): 2038 ms CPU
Thread 133669187532480 (treatImage): 2148 ms CPU
Thread 133669179139776 (treatImage): 2203 ms CPU
Thread 133669162354368 (treatImage): 2220 ms CPU
Thread 133669223741760 (main): 6 ms CPU
Total runtime (wall clock): 2333 ms
Memory usage: Resident: 136 MB, Peak: 364 MB
Total CPU time across all threads: 8615 ms
```
- N=8: 
```
./build/TME4 -m pipe_mt -n 8 -i input_images -o output_images
Thread 140065817220800 (treatImage): 1176 ms CPU
Thread 140065808828096 (treatImage): 1190 ms CPU
Thread 140065842398912 (treatImage): 1149 ms CPU
Thread 140065825613504 (treatImage): 1235 ms CPU
Thread 140065859184320 (treatImage): 1280 ms CPU
Thread 140065834006208 (treatImage): 1302 ms CPU
Thread 140065850791616 (treatImage): 1369 ms CPU
Thread 140065867577024 (treatImage): 1337 ms CPU
Thread 140065912465728 (main): 6 ms CPU
Total runtime (wall clock): 1444 ms
Memory usage: Resident: 189 MB, Peak: 570 MB
Total CPU time across all threads: 10044 ms
```

Best: 41 est pas mal un thread par fichier. Si on rajoute trop de threads c'est plus c'est lent la raison
est qu'on a bcp de contension avec le notify_all. 
Le temps total CPU augmente avec le nombre de threads
car chaque thread a un cout fixe.

## Question 6: TaskData struct

```cpp
struct TaskData {
    std::filesystem::path filePath;
    std::shared_ptr<QImage> image;

    TaskData(const std::filesystem::path& path, std::shared_ptr<QImage> img)
        : filePath(path), image(std::move(img)) {}
};
```

Fields: QImage ??? for the image data, ...

Use ??? for QImage, because ...

TASK_POISON: ...def...

## Question 7: ImageTaskQueue typing

pointers vs values

Choose BoundedBlockingQueue<TaskData*> as consequence
pour éviter les copies du contenu de la struct + unique_ptr on peut
pas le copier.

## Question 8: Pipeline functions

Implement reader, resizer, saver in Tasks.cpp.

mt_pipeline mode: Creates threads for each stage, with configurable numbers.

Termination: Main pushes the appropriate number of poisons after joining the previous stage.

Measurements: 
```
./build/TME4 -m mt_pipeline -i input_images -o output_images
Total runtime (wall clock): 1274 ms
Memory usage: Resident: 369 MB, Peak: 920 MB
Total CPU time across all threads: 11818 ms pr 16 threads
```


## Question 9: Configurable parallelism

Added nbread, nbresize, nbwrite options.


Timings:
- 1/1/1 (default): 
```
./build/TME4 -m mt_pipeline -i input_images -o output_images
...
```
- 1/4/1: 
```
./build/TME4 -m mt_pipeline --nbread 1 --nbresize 4 --nbwrite 1 -i input_images -o output_images
```

- 4/1/1: 
```
./build/TME4 -m mt_pipeline --nbread 4 --nbresize 1 --nbwrite 1 -i input_images -o output_images
```
... autres configs

Best config: 
interprétation

## Question 10: Queue sizes impact


With size 1: 
```
./build/TME4 -m pipe_mt -n 2 --queue-size 1 -i input_images -o output_images
...
```

With size 100: 
```
./build/TME4 -m pipe_mt -n 2 --queue-size 100 -i input_images -o output_images
...
```

impact

Complexity: 


## Question 11: BoundedBlockingQueueBytes

Implemented with byte limit.

mesures

## Question 12: Why important

Always allow push if current_bytes == 0, ...

Fairness: ...

## Bonus

