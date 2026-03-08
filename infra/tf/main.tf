provider "aws" {
  region  = "us-west-2"
  profile = "flight-path-admin"
}

# 1. IAM Role for the Lambda
resource "aws_iam_role" "iam_for_lambda" {
  name = "airport_board_lambda_role"

  assume_role_policy = jsonencode({
    Version = "2012-10-17"
    Statement = [{
      Action = "sts:AssumeRole"
      Effect = "Allow"
      Sid    = ""
      Principal = {
        Service = "lambda.amazonaws.com"
      }
    }]
  })
}

# Basic Execution Policy (for CloudWatch Logs)
resource "aws_iam_role_policy_attachment" "lambda_logs" {
  role       = aws_iam_role.iam_for_lambda.name
  policy_arn = "arn:aws:iam::aws:policy/service-role/AWSLambdaBasicExecutionRole"
}

# Install requirements in lambda
resource "null_resource" "pip_install" {
  triggers = {
    shell_hash = filesha256("${path.module}/../../requirements.txt")
  }

  provisioner "local-exec" {
    command = <<EOT
      mkdir -p ${path.module}/layer/python
      pip install -r ${path.module}/../../requirements.txt -t ${path.module}/layer/python
    EOT
  }
}
data "archive_file" "layer_zip" {
  type        = "zip"
  source_dir  = "${path.module}/layer"
  output_path = "${path.module}/layer.zip"

  depends_on = [null_resource.pip_install]
}

# This replaces the need for a manual zip script
data "archive_file" "lambda_zip" {
  type        = "zip"
  source_file = "${path.module}/../../src/main.py"
  output_path = "${path.module}/lambda_function_payload.zip"
}

resource "aws_lambda_function" "airport_fetcher" {
  filename         = data.archive_file.lambda_zip.output_path
  source_code_hash = data.archive_file.lambda_zip.output_base64sha256
  
  function_name = "lambda_handler"
  layers = [aws_lambda_layer_version.opensky_layer.arn]
  role          = aws_iam_role.iam_for_lambda.arn
  handler       = "main.lambda_handler" # Note: main matches main.py
  runtime       = "python3.11"
}

resource "aws_lambda_layer_version" "opensky_layer" {
  filename            = data.archive_file.layer_zip.output_path
  source_code_hash    = data.archive_file.layer_zip.output_base64sha256
  layer_name          = "airport_board_dependencies"
  compatible_runtimes = ["python3.11"]
}

# The Function URL (This makes it reachable via HTTPS)
resource "aws_lambda_function_url" "endpoint" {
  function_name      = aws_lambda_function.airport_fetcher.function_name
  authorization_type = "NONE" # Publicly accessible for now
}

# Output the URL to use in your Arduino code
output "lambda_url" {
  value = aws_lambda_function_url.endpoint.function_url
}