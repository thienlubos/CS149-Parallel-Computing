from pathlib import Path
import matplotlib.pyplot as plt
import argparse

SPEED_RESULTS_NAME = ["speed_results_ispc.txt",
                        "speed_results_task_ispc.txt",
                        "speed_results_AVX2.txt"]

def plot_speed_results(data, image_name):
    x, y = zip(*data)
    
    plt.figure(figsize=(10, 6))
    plt.plot(x, y, marker='o', linestyle='-', color='b')
    plt.title('Speed Results of Task ISPC')
    plt.xlabel('Init values')
    plt.ylabel('Speed up ratio')
    plt.grid(True)
    plt.tight_layout()

    plt.savefig(image_name)
    plt.close()

def plot_all_speed_results(data, image_name):
    x_ispc, y_ispc = zip(*data[0])
    x_task_ispc, y_task_ispc = zip(*data[1])
    x_avx2, y_avx2 = zip(*data[2])
    
    plt.figure(figsize=(10, 6))
    plt.plot(x_ispc, y_ispc, marker='o', linestyle='-', color='b', label='ISPC')
    plt.plot(x_task_ispc, y_task_ispc, marker='o', linestyle='-', color='r', label='Task ISPC')
    plt.plot(x_avx2, y_avx2, marker='o', linestyle='-', color='g', label='AVX2')
    plt.title('Speed Results of ISPC, Task ISPC and AVX2')
    plt.xlabel('Init values')
    plt.ylabel('Speed up ratio')
    plt.legend()
    plt.grid(True)
    plt.tight_layout()

    plt.savefig(image_name)
    plt.close()


def read_results(file_name) -> list:
    data = []
    try:
        with open(file_name, "r") as f: 
            lines = f.readlines()
        
        for line in lines:
            split_data = line.split(" ")
            data_point = [float(split_data[0]), float(split_data[1].replace("\n", ""))]
            data.append(data_point)
        
        print(f"Successfully read data from file {file_name} with {len(data)} data points")

        return data

    except Exception as e:
        print(f"Error while read data from file {file_name}")
        print(e)

        return None
    

if __name__=="__main__":

    # Instantiate the parser
    parser = argparse.ArgumentParser(description='Optional app description')
    parser.add_argument('-d', '--dir', default='results', help='Directory argument')
    args = parser.parse_args()

    asst_path = Path(__file__).resolve().parent.parent
    result_path : Path = asst_path / args.dir

    result_path.mkdir(exist_ok=True)


    print(f"Result dir: {result_path}")
    # for name in SPEED_RESULTS_NAME:
    #     file_name = result_path / name
    #     data = read_results(file_name)
        
    #     if (data != None):
    #         if (len(data) <= 1): 
    #             print(f"Data is not enough to plot an image, at least 2 data points is required, len(data) = {len(data)}")
    #         else:
    #             image_name = result_path / f"{name}_{len(data)}.png"
    #             plot_speed_results(data, image_name)
    #             print(f"Finish plot results, saved to image {file_name}")
    #     else:
    #         print(f"Failed to load data from {file_name}")

    data = []
    for name in SPEED_RESULTS_NAME:
        file_name = result_path / name
        data.append(read_results(file_name))
        
    if (data != None):
        if (len(data) <= 1): 
            print(f"Data is not enough to plot an image, at least 2 data points is required, len(data) = {len(data)}")
        else:
            image_name = result_path / f"speed_results_all.png"
            plot_all_speed_results(data, image_name)
            print(f"Finish plot results, saved to image {file_name}")
    else:
        print(f"Failed to load data from {file_name}")