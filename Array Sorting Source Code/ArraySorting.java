/**
 * Created by Kevin Weiyi Zhang
 * This is to demonstrate my understanding of how these sorting algorithms work
 */
import java.util.Random;
import java.util.Scanner;

public class ArraySorting {

    private int[] array;

    public ArraySorting() {
        generateUnsortedArray();
    }

    public void generateUnsortedArray() {
        Random random = new Random();
        array = new int[random.nextInt(50) + 1];    //size can be from 1-50 inclusive
        for (int i = 0; i < array.length; i++) {
            array[i] = random.nextInt(101); //values can be from 0-100 inclusive
        }
    }

    public void printArray() {
        for (int i : array) {
            System.out.print(i + " ");
        }
        System.out.println();
    }

    /**
     * Selection sort: scan array, find the min, swap with current location
     */
    public void selectionSort() {
        //outer loop keeps track of value to potentially be swapped
        for (int i1 = 0; i1 < array.length; i1++) {
            int minIndex = i1;
            //inner loop searches for min value
            for (int i2 = i1; i2 < array.length; i2++) {
                if (array[i2] < array[minIndex]) {
                    minIndex = i2;
                }
            }
            //swap
            int temp = array[i1];
            array[i1] = array[minIndex];
            array[minIndex] = temp;
//            printArray();   //uncomment if you want to see step by step
        }
    }

    /**
     * Insertion sort: scan array, swap values so they are in ascending order
     */
    public void insertionSort() {
        for (int i = 0; i < array.length; i++) {
            int swappingIndex = i;
            //swaps until values up to the index are in increasing order
            while (swappingIndex > 0 && array[swappingIndex] < array[swappingIndex - 1]) {
                int temp = array[swappingIndex - 1];
                array[swappingIndex - 1] = array[swappingIndex];
                array[swappingIndex] = temp;
                swappingIndex--;
//                printArray();   //uncomment if you want to see step by step
            }
        }
    }

    /**
     * Merge sort: Recursively splits array in half, then sorts values in increasing order
     */
    public void mergeSort() {
        array = splitThenMerge(array);
    }

    public int[] splitThenMerge(int[] a) {
        if (a.length == 1) {
            return a;
        }
        //split the array into 2 parts
        int[] half1;
        int[] half2;
        if (a.length % 2 == 1) {
            half1 = new int[(a.length / 2) + 1];
        }
        else {
            half1 = new int[a.length / 2];
        }
        half2 = new int[a.length / 2];
        for (int i = 0; i < half1.length; i++) {
            half1[i] = a[i];
        }
        for (int i = 0; i < half2.length; i++) {
            half2[i] = a[i + half1.length];
        }
        //recursively split
        half1 = splitThenMerge(half1);
        half2 = splitThenMerge(half2);
        //merge
        return merge(half1, half2);
    }

    public int[] merge(int[] a1, int[] a2) {
        int[] merged = new int[a1.length + a2.length];
        int i = 0;
        int i1 = 0;
        int i2 = 0;
        //add to "merge" until one of the arrays is empty
        while (i1 != a1.length && i2 != a2.length) {
            if (a1[i1] < a2[i2]) {
                merged[i] = a1[i1];
                i1++;
            }
            else {
                merged[i] = a2[i2];
                i2++;
            }
            i++;
        }
        //put in remaining elements
        for (; i1 < a1.length; i1++) {
            merged[i] = a1[i1];
            i++;
        }
        for (; i2 < a2.length; i2++) {
            merged[i] = a2[i2];
            i++;
        }
        //uncomment below to see step by step
//        for (int x : merged)
//            System.out.print(x + " ");
//        System.out.println();
        return merged;
    }

    public static void main(String[] args) {
        ArraySorting sort = new ArraySorting();
        System.out.println("Array Sorting Algorithms");
        System.out.println("Enter 'newRandomArray' to generate an unsorted array of ints");
        System.out.println("Enter 'print' to print the array");
        System.out.println("Enter 'selectionSort' or 'insertionSort' or 'mergeSort' to sort the array");
        System.out.println("Enter 'quit' to quit the program");
        Scanner input = new Scanner(System.in);
        String cmd = "";
        while (!cmd.equals("quit")) {
            cmd = input.nextLine();
            switch (cmd) {
                case "quit":
                    break;
                case "print":
                    sort.printArray();
                    break;
                case "newRandomArray":
                    sort.generateUnsortedArray();
                    break;
                case "selectionSort":
                    sort.selectionSort();
                    break;
                case "insertionSort":
                    sort.insertionSort();
                    break;
                case "mergeSort":
                    sort.mergeSort();
                    break;
            }
        }

    }
}
