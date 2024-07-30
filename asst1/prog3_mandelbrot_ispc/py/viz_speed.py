from pathlib import Path
import matplotlib.pyplot as plt
import argparse

SPEED_RESULTS_NAME = "speed_results_task_ispc.txt"

def plot_speed_results(data, image_name):
    x, y = zip(*data)
    
    plt.figure(figsize=(10, 6))
    plt.plot(x, y, marker='o', linestyle='-', color='b')
    plt.title('Speed Results')
    plt.xlabel('Number of Tasks')
    plt.ylabel('Speed up ratio')
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
            data_point = [int(split_data[0]), float(split_data[1].replace("\n", ""))]
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
    file_name = result_path / SPEED_RESULTS_NAME
    data = read_results(file_name)
    
    if (data != None):
        if (len(data) <= 1): 
            print(f"Data is not enough to plot an image, at least 2 data points is required, len(data) = {len(data)}")
        else:
            image_name = result_path / f"{SPEED_RESULTS_NAME}_{len(data)}.png"
            plot_speed_results(data, image_name)
            print(f"Finish plot results, saved to image {file_name}")
    else:
        print(f"Failed to load data from {file_name}")