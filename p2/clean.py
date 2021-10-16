for i in range(2,9):
    file = open("results/openmpresults/sense_reversal_outputs/omp-example-" + str(i) + "t-log.out","r")
    newfile = open("results/openmpresults/sense_reversal_outputs/cleaned-omp-example-"+str(i)+"t-log.out", "x")
    newfile.write("Barrier,ThreadID,Time\n")
    Lines = file.readlines()
    for line in Lines:
        if len(line.split(',')) == 3:
            newfile.write(line)
    file.close()
    newfile.close()
